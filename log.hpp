#pragma once

#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace trivial = boost::log::trivial;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(GlobalLogger, boost::log::sources::wseverity_logger_mt<trivial::severity_level>)
