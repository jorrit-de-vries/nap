#include "thememanager.h"
#include "appcontext.h"
#include "napkinglobals.h"
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <nap/logger.h>
#include <QFontDatabase>

using namespace napkin;

ThemeManager::ThemeManager() : mCurrentTheme(TXT_DEFAULT_THEME)
{
	loadFonts();

	connect(&mFileWatcher, &QFileSystemWatcher::directoryChanged, this, &ThemeManager::onFileChanged);
	connect(&mFileWatcher, &QFileSystemWatcher::fileChanged, this, &ThemeManager::onFileChanged);

	mFileWatcher.addPath(getThemeDir());
}


void ThemeManager::setTheme(const QString& themeName)
{
	if (themeName.isEmpty())
	{
		mCurrentTheme = napkin::TXT_DEFAULT_THEME;
	}
	else
	{
		auto theme_filename = getThemeFilename(themeName);
		if (!QFileInfo::exists(theme_filename))
		{
			nap::Logger::warn("File not found: %s", theme_filename.toStdString().c_str());
			return;
		}

		mCurrentTheme = themeName;
	}

	reloadTheme();
	QSettings().setValue(settingsKey::LAST_THEME, mCurrentTheme);
	themeChanged(mCurrentTheme);
}

const QString& ThemeManager::getCurrentTheme() const
{
	return mCurrentTheme;
}

QStringList ThemeManager::getAvailableThemes()
{
	QStringList names;

	for (const auto& filename : QDir(getThemeDir()).entryInfoList())
	{
		if (filename.suffix() == sThemeFileExtension)
			names << filename.baseName();
	}
	return names;
}

const QString ThemeManager::getThemeDir() const
{
	// TODO: This probably needs to be configurable
	return QString("%1/%2").arg(QCoreApplication::applicationDirPath(), sThemeSubDirectory);
}

void ThemeManager::reloadTheme()
{
	if (mCurrentTheme == TXT_DEFAULT_THEME)
	{
		AppContext::get().getQApplication()->setStyleSheet(nullptr);
		return;
	}

	if (mCurrentTheme.isEmpty())
	{
		nap::Logger::warn("No theme set, not reloading");
		return;
	}

	auto theme_filename = getThemeFilename(mCurrentTheme);
	QFile file(theme_filename);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		nap::Logger::warn("Could not load file: %s", theme_filename.toStdString().c_str());
		return;
	}

	QTextStream in(&file);
	auto styleSheet = in.readAll();
	file.close();

	mFileWatcher.addPath(theme_filename);

	AppContext::get().getQApplication()->setStyleSheet(styleSheet);

}

const QString ThemeManager::getThemeFilename(const QString& themeName) const
{
	return QString("%1/%2.%3").arg(getThemeDir(), themeName, sThemeFileExtension);
}

void ThemeManager::onFileChanged(const QString& path)
{
	auto theme_filename = getThemeFilename(mCurrentTheme);
	QFileInfo path_info(path);
	if (path_info.filePath() == theme_filename) {
		nap::Logger::info("Reloading: %s", path.toStdString().c_str());
		reloadTheme();
	}

	mFileWatcher.addPath(theme_filename);
}

void ThemeManager::loadFonts()
{
	QStringList fonts;
	fonts << ":/fonts/Montserrat-ExtraBold.ttf";
	fonts << ":/fonts/Montserrat-Light.ttf";
	fonts << ":/fonts/Montserrat-Medium.ttf";
	fonts << ":/fonts/NunitoSans-ExtraBold.ttf";
	fonts << ":/fonts/NunitoSans-Regular.ttf";
	fonts << ":/fonts/NunitoSans-SemiBold.ttf";

	for (auto font : fonts)
	{
		int id = QFontDatabase::addApplicationFont(font);
		if (id < 0)
		{
			nap::Logger::warn("Failed to load font: '%s'", font.toStdString().c_str());
		}
	}

}