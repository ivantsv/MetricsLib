#pragma once

#include <type_traits>
#include <typeinfo>
#include <memory> 
#include <utility>
#include <any>

namespace MyCustomAny {
namespace detail {

struct IAnyConcept {
    virtual ~IAnyConcept() = default;

    virtual const std::type_info& type() const noexcept = 0;
    virtual std::unique_ptr<IAnyConcept> clone() const = 0;
    virtual bool equals(const IAnyConcept& other) const = 0;
};

template <typename T>
struct AnyModel final : public IAnyConcept {
    T value; 

    explicit AnyModel(const T& val) : value(val) {}
    explicit AnyModel(T&& val) : value(std::move(val)) {}

    const std::type_info& type() const noexcept override {
        return typeid(T);
    }
    
    std::unique_ptr<IAnyConcept> clone() const override {
        return std::make_unique<AnyModel<T>>(value);
    }

    bool equals(const IAnyConcept& other) const override {
        if (typeid(T) != other.type()) {
            return false;
        }

        const AnyModel<T>& other_model = static_cast<const AnyModel<T>&>(other);

        return value == other_model.value;
    }
};

}

class MyAny {
public:
    MyAny() noexcept = default;

    MyAny(const MyAny& other) {
        if (other.has_value()) {
            concept_ptr_ = other.concept_ptr_->clone();
        }
    }

    MyAny(MyAny&& other) noexcept : concept_ptr_(std::move(other.concept_ptr_)) {}

    template <typename T>
    requires (!std::is_same_v<std::decay_t<T>, MyAny>)
    MyAny(T&& value) : concept_ptr_(std::make_unique<detail::AnyModel<std::remove_reference_t<T>>>(std::forward<T>(value))) {}

    MyAny& operator=(const MyAny& other) {
        if (this != &other) {
            MyAny temp(other);
            *this = std::move(temp);
        }
        return *this;
    }

    MyAny& operator=(MyAny&& other) noexcept {
        concept_ptr_ = std::move(other.concept_ptr_);
        return *this;
    }

    void reset() noexcept {
        concept_ptr_.reset();
    }

    bool has_value() const noexcept {
        return concept_ptr_ != nullptr;
    }

    const std::type_info& type() const noexcept {
        return has_value() ? concept_ptr_->type() : typeid(void);
    }

    bool operator==(const MyAny& rhs) const {
        if (!has_value() && !rhs.has_value()) {
            return true;
        }
 
        if (!has_value() || !rhs.has_value()) {
            return false;
        }

        if (type() != rhs.type()) {
            return false;
        }
        return concept_ptr_->equals(*rhs.concept_ptr_);
    }

private:
    std::unique_ptr<detail::IAnyConcept> concept_ptr_;
    
    template <typename ValueType>
    friend ValueType& MyAnyCast(MyAny& operand);
    
    template <typename ValueType>
    friend const ValueType& MyAnyCast(const MyAny& operand);
    
    template <typename ValueType>
    friend ValueType MyAnyCast(MyAny&& operand);
};

template <typename ValueType>
ValueType& MyAnyCast(MyAny& operand) {
    if (!operand.has_value() || operand.type() != typeid(ValueType)) {
        throw std::bad_any_cast();
    }

    return static_cast<detail::AnyModel<ValueType>*>(operand.concept_ptr_.get())->value;
}

template <typename ValueType>
const ValueType& MyAnyCast(const MyAny& operand) {
    if (!operand.has_value() || operand.type() != typeid(ValueType)) {
        throw std::bad_any_cast();
    }
    return static_cast<const detail::AnyModel<ValueType>*>(operand.concept_ptr_.get())->value;
}

template <typename ValueType>
ValueType MyAnyCast(MyAny&& operand) {
    if (!operand.has_value() || operand.type() != typeid(ValueType)) {
        throw std::bad_any_cast();
    }

    return std::move(static_cast<detail::AnyModel<ValueType>*>(operand.concept_ptr_.get())->value);
}

}