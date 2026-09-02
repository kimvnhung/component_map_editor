#include "AdapterCommon.h"

namespace cme::adapter {

ConversionError::ConversionError(const QString &msg)
    : has_error(true)
    , error_message(msg)
{
}

ConversionError::operator bool() const
{
    return has_error;
}

} // namespace cme::adapter
