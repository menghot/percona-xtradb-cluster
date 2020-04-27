/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "plugin/x/src/notices.h"

#include <functional>
#include <string>
#include <vector>

#include "plugin/x/ngs/include/ngs/protocol/protocol_protobuf.h"
#include "plugin/x/src/interface/protocol_encoder.h"
#include "plugin/x/src/interface/protocol_monitor.h"
#include "plugin/x/src/interface/sql_session.h"
#include "plugin/x/src/xpl_resultset.h"

namespace xpl {

namespace notices {

namespace {

class Warning_resultset : public Process_resultset {
 public:
  Warning_resultset(iface::Protocol_encoder *proto,
                    const bool skip_single_error)
      : m_proto(proto), m_skip_single_error(skip_single_error) {}

 protected:
  using Warning = Mysqlx::Notice::Warning;

  Row *start_row() override {
    m_row.clear();
    return &m_row;
  }

  Warning::Level get_warning_level(const std::string &level) {
    static const char *const ERROR_STRING = "Error";
    static const char *const WARNING_STRING = "Warning";
    if (level == WARNING_STRING) return Warning::WARNING;
    if (level == ERROR_STRING) return Warning::ERROR;
    return Warning::NOTE;
  }

  bool end_row(Row *row) override {
    if (!m_last_error.empty()) {
      m_proto->send_notice(iface::Frame_type::k_warning,
                           iface::Frame_scope::k_local, m_last_error);
      m_last_error.clear();
    }

    Field_list &fields = row->fields;
    if (fields.size() != 3) return false;

    const Warning::Level level = get_warning_level(*fields[0]->value.v_string);

    Warning warning;
    warning.set_level(level);
    warning.set_code(
        static_cast<google::protobuf::uint32>(fields[1]->value.v_long));
    warning.set_msg(*fields[2]->value.v_string);

    std::string data;
    warning.SerializeToString(&data);

    if (level == Warning::ERROR) {
      ++m_num_errors;
      if (m_skip_single_error && (m_num_errors <= 1)) {
        m_last_error = data;
        return true;
      }
    }

    m_proto->send_notice(iface::Frame_type::k_warning,
                         iface::Frame_scope::k_local, data);
    return true;
  }

 private:
  Row m_row;
  iface::Protocol_encoder *m_proto;
  const bool m_skip_single_error;
  std::string m_last_error;
  uint32_t m_num_errors{0u};
};

}  // namespace

ngs::Error_code send_warnings(iface::Sql_session &da,
                              iface::Protocol_encoder &proto,
                              bool skip_single_error) {
  DBUG_TRACE;
  static const std::string q = "SHOW WARNINGS";
  Warning_resultset resultset(&proto, skip_single_error);
  // send warnings as notices
  return da.execute(q.data(), q.length(), &resultset);
}

}  // namespace notices
}  // namespace xpl
