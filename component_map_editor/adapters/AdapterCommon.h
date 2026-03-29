// component_map_editor/adapters/AdapterCommon.h
//
// Shared types and utilities for all adapters.

#pragma once

#include <QString>

namespace cme::adapter {

/// Result of a conversion operation: either success or an error message.
struct ConversionError {
    bool has_error = false;
    QString error_message;

    ConversionError() = default;
    ConversionError(const QString &msg) : has_error(true), error_message(msg) {}

    explicit operator bool() const { return has_error; }
};

} // namespace cme::adapter
