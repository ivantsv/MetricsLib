#pragma once

#include <string>
#include <memory>

class IMetric {
public:
    virtual ~IMetric() = default;
    virtual std::string getName() const = 0;
    virtual std::string getValueAsString() const = 0;
    virtual void evaluate() = 0;
    virtual void reset() = 0;
};

