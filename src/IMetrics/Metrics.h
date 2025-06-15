#pragma once

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-variable"
    #pragma GCC diagnostic ignored "-Wunused-function"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
    #pragma GCC diagnostic ignored "-Wunused-private-field"
    #pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    #pragma GCC diagnostic ignored "-Wunused-label"
#elif _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4100)
    #pragma warning(disable: 4101)
    #pragma warning(disable: 4189)
    #pragma warning(disable: 4505)
#endif

#include "CardinalityMetricType.h"
#include "CardinalityMetricValue.h"
#include "CodeTimeMetric.h"
#include "CPUUsageMetric.h"
#include "CPUUtilMetric.h"
#include "HTTPIncomeMetric.h"
#include "IncrementMetric.h"
#include "LatencyMetric.h"