#pragma once

#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace bltriv = boost::log::trivial;
namespace blog = boost::log;
namespace blkw = boost::log::keywords;
namespace blexpr = boost::log::expressions;
namespace blattr = boost::log::attributes;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(GlobalLogger, boost::log::sources::wseverity_logger_mt<bltriv::severity_level>)