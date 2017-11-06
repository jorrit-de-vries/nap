#include "appcontext.h"
#include <QSettings>
#include <rtti/jsonreader.h>
#include <rtti/jsonwriter.h>
#include <fstream>
#include <QtWidgets/QMessageBox>

using namespace nap::rtti;
using namespace nap::utility;

bool ResolveLinks(const OwnedObjectList& objects, const UnresolvedPointerList& unresolvedPointers)
{
    std::map<std::string, RTTIObject*> objects_by_id;
    for (auto& object : objects)
        objects_by_id.insert({object->mID, object.get()});

    for (const UnresolvedPointer& unresolvedPointer : unresolvedPointers) {
        ResolvedRTTIPath resolved_path;
        if (!unresolvedPointer.mRTTIPath.resolve(unresolvedPointer.mObject, resolved_path))
            return false;

        auto pos = objects_by_id.find(unresolvedPointer.mTargetID);
        if (pos == objects_by_id.end())
            return false;

        if (!resolved_path.setValue(pos->second))
            return false;
    }

    return true;
}

AppContext::AppContext()
{
}


AppContext& AppContext::get()
{
    static AppContext inst;
    return inst;
}

void AppContext::newFile()
{
    mCurrentFilename = "";
    mObjects.clear();
    newFileCreated();
}


void AppContext::loadFile(const QString& filename)
{
    mCurrentFilename = filename;
    QSettings settings;
    settings.setValue(LAST_OPENED_FILE, filename);

    auto& factory = core().getResourceManager()->getFactory();
    ErrorState err;
    nap::rtti::RTTIDeserializeResult result;
    if (!readJSONFile(filename.toStdString(), factory, result, err)) {
        nap::Logger::fatal(err.toString());
        return;
    }

    if (!ResolveLinks(result.mReadObjects, result.mUnresolvedPointers)) {
        nap::Logger::fatal("Failed to resolve links");
        return;
    }

    // transfer
    mObjects.clear();
    for (auto& ob : result.mReadObjects) {
        mObjects.emplace_back(std::move(ob));
    }

    fileOpened(mCurrentFilename);
}

void AppContext::saveFile()
{
    saveFileAs(mCurrentFilename);
}

void AppContext::saveFileAs(const QString& filename)
{
    ObjectList objects;
    for (auto& ob : mObjects) {
        objects.emplace_back(ob.get());
    }

    JSONWriter writer;
    ErrorState err;
    if (!serializeObjects(objects, writer, err)) {
        nap::Logger::fatal(err.toString());
        return;
    }

    std::ofstream out(filename.toStdString());
    out << writer.GetJSON();
    out.close();

    mCurrentFilename = filename;
    nap::Logger::info("Written file: " + filename.toStdString());

    fileSaved(mCurrentFilename);
}


const QString AppContext::lastOpenedFilename()
{
    QSettings settings;
    return settings.value(LAST_OPENED_FILE).toString();
}


nap::Entity* AppContext::getParent(const nap::Entity& child)
{
    for (const auto& o : objects()) {
        if (!o->get_type().is_derived_from<nap::Entity>())
            continue;

        auto parent = dynamic_cast<nap::Entity*>(o.get());
        auto it = std::find_if(parent->mChildren.begin(), parent->mChildren.end(),
                               [&child](nap::ObjectPtr<nap::Entity> e) -> bool {
                                   return &child == e.get();
                               });

        if (it != parent->mChildren.end())
            return parent;
    }
    return nullptr;
}

nap::Entity* AppContext::getOwner(const nap::Component& component)
{
    for (const auto& o : objects()) {
        if (!o->get_type().is_derived_from<nap::Entity>())
            continue;

        auto owner = dynamic_cast<nap::Entity*>(o.get());
        auto it = std::find_if(owner->mComponents.begin(), owner->mComponents.end(),
                               [&component](nap::ObjectPtr<nap::Component> comp) -> bool {
                                   return &component == comp.get();
                               });

        if (it != owner->mComponents.end())
            return owner;
    }
    return nullptr;
}


nap::Entity* AppContext::createEntity(nap::Entity* parent)
{
    auto e = std::make_unique<nap::Entity>();
    e->mID = getUniqueName("New Entity");
    auto ret = e.get();
    mObjects.emplace_back(std::move(e));

    if (parent != nullptr) {
        parent->mChildren.emplace_back(ret);
    }

    entityAdded(ret, parent);
    return ret;
}

nap::Component* AppContext::addComponent(nap::Entity& entity, rttr::type type)
{
    assert(type.can_create_instance());
    assert(type.is_derived_from<nap::Component>());

    auto compVariant = type.create();
    auto comp = compVariant.get_value<nap::Component*>();
    comp->mID = getUniqueName(type.get_name().data());
    mObjects.emplace_back(comp);
    entity.mComponents.emplace_back(comp);

    componentAdded(*comp, entity);

    return comp;
}


nap::rtti::RTTIObject* AppContext::addObject(rttr::type type)
{
    assert(type.can_create_instance());
    assert(type.is_derived_from<nap::rtti::RTTIObject>());
    auto variant = type.create();
    auto obj = variant.get_value<nap::rtti::RTTIObject*>();
    obj->mID = getUniqueName(type.get_name().data());
    mObjects.emplace_back(obj);
    objectAdded(*obj);
    return obj;
}





std::string AppContext::getUniqueName(const std::string& suggestedName)
{
    std::string newName = suggestedName;
    int i = 2;
    while (getObject(newName))
        newName = suggestedName + "_" + std::to_string(i++);
    return newName;
}

nap::rtti::RTTIObject* AppContext::getObject(const std::string& name)
{
    auto it = std::find_if(mObjects.begin(), mObjects.end(), [&name](std::unique_ptr<nap::rtti::RTTIObject>& obj) {
        return obj->mID == name;
    });
    if (it == mObjects.end())
        return nullptr;
    return it->get();
}

void AppContext::deleteObject(nap::rtti::RTTIObject& object)
{
    if (object.get_type().is_derived_from<nap::Entity>()) {
        auto parent = getParent(dynamic_cast<nap::Entity&>(object));
        if (parent)
            parent->mChildren.erase(std::remove(parent->mChildren.begin(), parent->mChildren.end(), &object));
    } else if (object.get_type().is_derived_from<nap::Component>()) {
        auto owner = getOwner(dynamic_cast<nap::Component&>(object));
        if (owner)
            owner->mComponents.erase(std::remove(owner->mComponents.begin(), owner->mComponents.end(), &object));
    }

    mObjects.erase(std::remove_if(mObjects.begin(), mObjects.end(),
                                  [&object](std::unique_ptr<nap::rtti::RTTIObject>& obj) {
                                      return obj.get() == &object;
                                  }), mObjects.end());

    objectRemoved(object);
}

void AppContext::executeCommand(QUndoCommand* cmd)
{
    mUndoStack.push(cmd);
}

