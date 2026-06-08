#include "runtime.hpp"
#include "interpreter.hpp"

UserFunction::UserFunction(std::shared_ptr<CompiledFunction> compiledFn,
                           std::shared_ptr<Environment> closure,
                           std::shared_ptr<UserClass> definingClass)
    : compiledFn(compiledFn), closure(closure), definingClass(definingClass) {
}

std::shared_ptr<UserFunction> UserFunction::bind(std::shared_ptr<Instance> instance) {
    auto environment = std::make_shared<Environment>(closure);
    RuntimeValue instanceValue;
    if (instance->superclassContext == nullptr) {
        auto boundInstance  = std::make_shared<Instance>(*instance, definingClass);
        instanceValue.value = boundInstance;
    } else {
        instanceValue.value = instance;
    }
    environment->define("this", instanceValue);
    return std::make_shared<UserFunction>(compiledFn, environment, definingClass);
}

int UserFunction::arity() {
    return compiledFn->arity;
}

RuntimeValue UserFunction::call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) {
    return interpreter.runFunction(compiledFn, arguments, closure);
}

std::string UserFunction::toString() {
    return "<FUNCTION " + compiledFn->name + ">";
}

NativeFunction::NativeFunction(int arity, NativeFn function) : function(function), _arity(arity) {
}

int NativeFunction::arity() {
    return _arity;
}

RuntimeValue NativeFunction::call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) {
    return function(interpreter, arguments);
}

std::string NativeFunction::toString() {
    return "<NATIVE FUNCTION>";
}

UserClass::UserClass(const std::string &n, std::shared_ptr<UserClass> s) : name(n), superclass(s) {
}

std::shared_ptr<UserClass> UserClass::getSuperclass() const {
    return superclass;
}

std::string UserClass::getName() const {
    return name;
}

void UserClass::addMethod(const std::string &methodName, std::shared_ptr<Callable> method) {
    methods[methodName] = method;
}

void UserClass::addField(const std::string &fieldName,
                         std::shared_ptr<CompiledFunction> valueFunc) {
    defaultFields[fieldName] = valueFunc;
}

std::shared_ptr<UserFunction> UserClass::findMethod(const std::string &methodName) {
    if (methods.count(methodName)) {
        return std::dynamic_pointer_cast<UserFunction>(methods.at(methodName));
    }
    if (superclass != nullptr) {
        return superclass->findMethod(methodName);
    }
    return nullptr;
}

std::shared_ptr<UserFunction> UserClass::findConstructor() {
    std::shared_ptr<UserFunction> constructor = findMethod(name);
    if (constructor != nullptr)
        return constructor;
    if (superclass != nullptr) {
        return superclass->findConstructor();
    }
    return nullptr;
}

int UserClass::arity() {
    auto constructor = findConstructor();
    if (constructor != nullptr)
        return constructor->arity();
    return 0;
}

RuntimeValue UserClass::call(Interpreter &interpreter, std::vector<RuntimeValue> arguments) {
    auto instance =
        std::make_shared<Instance>(std::static_pointer_cast<Callable>(shared_from_this()));
    initializeFields(interpreter, instance);
    auto constructor = findConstructor();
    if (constructor != nullptr) {
        constructor->bind(instance)->call(interpreter, arguments);
    }
    return RuntimeValue{instance};
}

void UserClass::initializeFields(Interpreter &interpreter, std::shared_ptr<Instance> instance) {
    if (superclass != nullptr) {
        superclass->initializeFields(interpreter, instance);
    }
    for (const auto &[key, func] : defaultFields) {
        (*instance->fields)[key] = interpreter.runFunction(func, {}, nullptr);
    }
}

std::string UserClass::toString() {
    return "<CLASS " + name + ">";
}
