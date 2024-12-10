// Automatically generated file: DO NOT MODIFY!

foreign class Vec2 {
    construct new (arg0, arg1) {}

    foreign !=(rhs)
    foreign static Default()
    foreign +(rhs)
    foreign -(rhs)
    foreign -
    foreign *(rhs)
    foreign ==(rhs)
    foreign normalize()
    foreign length()
    foreign x
    foreign x=(rhs)
    foreign y
    foreign y=(rhs)
}

foreign class NameComponent {
    foreign name
}

foreign class Entity {
    foreign GetTransformComponent()
    foreign AddTransformComponent()
    foreign GetNameComponent()
    foreign AddNameComponent()
}

foreign class Vec3 {
    construct new (arg0, arg1, arg2) {}

    foreign !=(rhs)
    foreign static Default()
    foreign +(rhs)
    foreign -(rhs)
    foreign -
    foreign *(rhs)
    foreign ==(rhs)
    foreign normalize()
    foreign length()
    foreign x
    foreign x=(rhs)
    foreign y
    foreign y=(rhs)
    foreign z
    foreign z=(rhs)
}

foreign class ECS {
    foreign NewEntity()
    foreign GetEntityByName(arg0)
    foreign DestroyEntity(arg0)
}

foreign class Quat {
    construct new (arg0, arg1, arg2, arg3) {}

    foreign !=(rhs)
    foreign static Default()
    foreign +(rhs)
    foreign -(rhs)
    foreign -
    foreign *(rhs)
    foreign ==(rhs)
    foreign normalize()
    foreign length()
    foreign w
    foreign w=(rhs)
    foreign x
    foreign x=(rhs)
    foreign y
    foreign y=(rhs)
    foreign z
    foreign z=(rhs)
}

foreign class Engine {
    foreign GetTime()
    foreign GetECS()
}

foreign class TimeModule {
    foreign GetDeltatime()
}

foreign class TransformComponent {
    foreign translation
    foreign translation=(rhs)
    foreign rotation
    foreign rotation=(rhs)
    foreign scale
    foreign scale=(rhs)
}


