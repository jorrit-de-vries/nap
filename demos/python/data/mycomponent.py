import nap

class MyComponent:

    def __init__(self, entity):
        self.value = 0
        self.entity = entity

    def update(self, currentTime, deltaTime):
        print("My value is " + str(self.value))

    def destroy(self):
        pass

    def setValue(self, value):
        print("setting value " + str(value))
        self.value = value

    def getValue(self):
        return self.value
