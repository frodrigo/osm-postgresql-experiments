
#include "options.hpp"
#include "table.hpp"

#include <iostream>
#include <math.h>

extern Options opts;

static const std::vector<stream_config_type> stream_config{
    {"o",  "objects",        "o.I.v.c.t.i.u.T.x.y.N.M.", "o.I.v.d.c.t.i.u.T.x.y.N.M.", stream_type::objects,   osmium::osm_entity_bits::nwr},
    {"n",  "nodes",          "I.v.c.t.i.u.T.x.y.",       "I.v.d.c.t.i.u.T.x.y.",       stream_type::objects,   osmium::osm_entity_bits::node},
    {"w",  "ways",           "I.v.c.t.i.u.T.N.",         "I.v.d.c.t.i.u.T.N.",         stream_type::objects,   osmium::osm_entity_bits::way},
    {"r",  "relations",      "I.v.c.t.i.u.T.M.",         "I.v.d.c.t.i.u.T.M.",         stream_type::objects,   osmium::osm_entity_bits::relation},
    {"oT", "tags",           "o.I.TkTv",                 "o.I.v.TkTv",                 stream_type::tags,      osmium::osm_entity_bits::nwr},
    {"nT", "node_tags",      "I.TkTv",                   "I.v.TkTv",                   stream_type::tags,      osmium::osm_entity_bits::node},
    {"wT", "way_tags",       "I.TkTv",                   "I.v.TkTv",                   stream_type::tags,      osmium::osm_entity_bits::way},
    {"rT", "relation_tags",  "I.TkTv",                   "I.v.TkTv",                   stream_type::tags,      osmium::osm_entity_bits::relation},
    {"wN", "way_nodes",      "I.NsNi",                   "I.v.NsNi",                   stream_type::way_nodes, osmium::osm_entity_bits::way},
    {"rM", "members",        "I.MsMoMiMr",               "I.v.MsMoMiMr",               stream_type::members,   osmium::osm_entity_bits::relation},
    {"u",  "users",          "i.u.",                     "i.u.",                       stream_type::users,     osmium::osm_entity_bits::all},
};

using cft = column_type;

static const std::vector<column_config_type> column_config{
    {"o.", cft::objtype,         "objtype",      "CHAR(1) CHECK(objtype IN ('n', 'w', 'r'))", {}},
    {"I.", cft::id,              "id",           "BIGINT NOT NULL",                           {}},
    {"v.", cft::version,         "version",      "INTEGER NOT NULL",                          {}},
    {"vI", cft::version,         "version",      "BIGINT NOT NULL",                           {}},
    {"d.", cft::deleted,         "deleted",      "BOOLEAN NOT NULL",                          {}},
    {"d!", cft::visible,         "visible",      "BOOLEAN NOT NULL",                          {}},
    {"c.", cft::changeset,       "changeset_id", "INTEGER NOT NULL",                          {}},
    {"cI", cft::changeset,       "changeset_id", "BIGINT NOT NULL",                           {}},
    {"t.", cft::timestamp_iso,   "created",      "TIMESTAMP (0) WITHOUT TIME ZONE",           {}},
    {"tu", cft::timestamp_sec,   "created",      "INTEGER",                                   {}},
    {"tr", cft::timestamp_range, "trange",       "TSTZRANGE",                                 time_range},
    {"i.", cft::uid,             "uid",          "INTEGER",                                   {}},
    {"u.", cft::user,            "username",     "TEXT",                                      {}},

    {"T.", cft::tags_jsonb,      "tags",      "JSONB",             {}},
    {"Tj", cft::tags_jsonb,      "tags",      "JSONB",             {}},
    {"TJ", cft::tags_json,       "tags",      "JSON",              {}},
    {"Ts", cft::tag_seq,         "seq_no",    "INTEGER NOT NULL",  {}},
    {"Tk", cft::tag_key,         "key",       "TEXT NOT NULL",     {}},
    {"Tv", cft::tag_value,       "value",     "TEXT NOT NULL",     {}},
    {"T=", cft::tag_kv,          "tag",       "TEXT NOT NULL",     {}},

    {"x.", cft::lon_real,        "lon",       "REAL",              {}},
    {"xi", cft::lon_int,         "lon",       "INTEGER",           {}},
    {"y.", cft::lat_real,        "lat",       "REAL",              {}},
    {"yi", cft::lat_int,         "lat",       "INTEGER",           {}},
    {"q.", cft::tile,            "tile",      "BIGINT",            {}},

    {"N.", cft::nodes_array,     "nodes",     "BIGINT[]",          {}},
    {"Ns", cft::node_seq,        "seq_no",    "INT NOT NULL",      {}},
    {"Ni", cft::node_ref,        "ref",       "BIGINT NOT NULL",   {}},

    {"M.", cft::members_jsonb,   "members",   "JSONB",                                     {}},
    {"Mj", cft::members_jsonb,   "members",   "JSONB",                                     {}},
    {"MJ", cft::members_json,    "members",   "JSON",                                      {}},
    {"Ms", cft::member_seq,      "seq_no",    "INTEGER NOT NULL",                          {}},
    {"Mo", cft::member_type,     "objtype",   "CHAR(1) CHECK(objtype IN ('n', 'w', 'r'))", {}},
    {"Mi", cft::member_ref,      "ref",       "BIGINT NOT NULL",                           {}},
    {"Mr", cft::member_role,     "role",      "TEXT NOT NULL",                             {}},

    {"G.", cft::geometry,            "geom",  "GEOMETRY",                          geom_index},
    {"Gp", cft::geometry_point,      "geom",  "GEOMETRY(POINT, 4326)",             geom_index},
    {"Gl", cft::geometry_linestring, "geom",  "GEOMETRY(LINESTRING, 4326)",        sql_column_config_flags(geom_index | location_store)},
    {"GP", cft::geometry_polygon,    "geom",  "GEOMETRY(MULTIPOLYGON, 4326)",      sql_column_config_flags(geom_index | location_store)},

    {"r.", cft::redaction,           "redaction_id", "INTEGER", {}}
};

void print_streams() {
    for (const auto& config : stream_config) {
        std::cout << "    " << config.stream << ": " << config.name << '\n';
    }
}

static const stream_config_type& get_stream_config(const std::string& stream_string) {
    for (const auto& c : stream_config) {
        if (c.stream == stream_string) {
            return c;
        }
    }

    throw std::runtime_error{"Unknown stream type '" + stream_string + "'"};
}

static const column_config_type& get_column_config(const std::string& format_string) {
    for (const auto& c : column_config) {
        if (format_string == c.format_string) {
            return c;
        }
    }

    throw std::runtime_error{"unknown column config: " + format_string};
}

void Table::setup_columns() {
    if (m_columns_string.size() % 2 != 0) {
        throw std::runtime_error{"config unpaired"};
    }

    for (auto it = m_columns_string.begin(); it != m_columns_string.end();) {
        std::string cs;
        cs += *it++;
        cs += *it++;
        m_columns.emplace_back(get_column_config(cs));
        m_column_flags = static_cast<sql_column_config_flags>(m_column_flags | m_columns.back().flags);
    }
}

Table::Table(const std::string& filename, const stream_config_type& stream_config, const std::string& columns_string) :
    m_filename(filename),
    m_columns_string(columns_string),
    m_stream_config(&stream_config) {

    if (m_filename == "") { // no name means STDOUT
        m_name = m_stream_config->name;
        m_fd = 1;
    } else {
        const auto last_slash = m_filename.find_last_of('/');
        if (last_slash == std::string::npos) {
            m_path = ".";
            m_name =  m_filename;
        } else {
            m_path = m_filename.substr(0, last_slash);
            m_name =  m_filename.substr(last_slash + 1);
        }
        const auto first_dot = m_name.find_first_of('.');
        if (first_dot != std::string::npos) {
            m_name = m_name.substr(0, first_dot);
        } else {
            m_filename += ".pgcopy";
        }

        m_fd = ::open(m_filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (m_fd < 0) {
            throw std::runtime_error{"can't open file: " + m_filename};
        }
    }

    setup_columns();

    m_buffer.reserve(1000 * 1024);
}

void Table::flush() {
    if (!m_buffer.empty()) {
        const auto written = ::write(m_fd, m_buffer.data(), m_buffer.size());
        if (written < 0 || static_cast<std::size_t>(written) != m_buffer.size()) {
            throw std::runtime_error{"write error"};
        }
        m_buffer.resize(0);
    }
}

void Table::close() {
    if (m_fd != -1 && m_fd != 1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

std::string Table::sql_primary_key() const {
    std::string sql;

    // TODO: should be different for different streams, disable if the fields are not all there
    std::string primary_keys;
    if (m_stream_config->entities == osmium::osm_entity_bits::nwr) {
        primary_keys += "objtype, ";
    }
    primary_keys += "id, ";
    if (opts.with_history) {
        primary_keys += "version, ";
    }
    primary_keys.resize(primary_keys.size() - 2);

    sql += "-- ALTER TABLE \"";
    sql += m_name;
    sql += "\" ADD PRIMARY KEY(" + primary_keys + ");\n";

    return sql;
}

void Table::sql_data_definition() const {
    std::string sql;

    sql += "\\timing\n\n";
    sql += "DROP TABLE IF EXISTS \"";
    sql += m_name;
    sql += "\" CASCADE;\n\n";

    sql += "CREATE TABLE \"";
    sql += m_name;
    sql += "\" (\n";

    for (const auto column : m_columns) {
        sql += "    \"";
        sql += column.sql_name;
        sql += "\" ";
        sql += column.sql_type;
        sql += ",\n";
    }

    sql.resize(sql.size() - 2);
    sql += "\n);\n\n";

    sql += "\\copy \"";
    sql += m_name;
    sql += "\" from '";
    sql += m_filename;
    sql += "'\n\n";

    sql += "ANALYZE \"";
    sql += m_name;
    sql += "\";\n\n";

    if (opts.with_primary_key) {
        sql += sql_primary_key();
    }

    if (m_column_flags & sql_column_config_flags::geom_index) {
        sql += "-- CREATE INDEX \"" + m_name + "_geom_idx\" ON \"" + m_name + "\" USING GIST (geom);\n";
    }

    sql += '\n';

    std::string sqlfilename{m_path + "/" + m_name + ".sql"};
    try {
        std::ofstream sqlfile{sqlfilename};
        sqlfile.exceptions(~std::ofstream::goodbit);
        sqlfile << sql;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error writing to file '" << sqlfilename << "'\n";
        throw;
    }
}

template <typename TFunc>
void append_coordinate(const osmium::OSMObject& object, std::string& buffer, TFunc&& func) {
    if (object.type() != osmium::item_type::node || !static_cast<const osmium::Node&>(object).location()) {
        buffer += "\\N";
        return;
    }

    const auto location = static_cast<const osmium::Node&>(object).location();
    buffer.append(std::forward<TFunc>(func)(location));
}

static inline unsigned int lon2x(double lon) noexcept {
   return std::round((lon + 180.0) * 65535.0 / 360.0);
}

static inline unsigned int lat2y(double lat) noexcept {
   return std::round((lat + 90.0) * 65535.0 / 180.0);
}

static inline unsigned int xy2tile(unsigned int x, unsigned int y) noexcept {
   unsigned int tile = 0;

   for (int i = 15; i >= 0; --i) {
      tile = (tile << 1u) | ((x >> i) & 1u);
      tile = (tile << 1u) | ((y >> i) & 1u);
   }

   return tile;
}

static inline unsigned int quadtile(const osmium::Location location) noexcept {
    return xy2tile(lon2x(location.lon()), lat2y(location.lat()));
}

void ObjectsTable::add_row(const osmium::OSMObject& object, const osmium::Timestamp next_version_timestamp) {
    for (const auto& column : m_columns) {
        switch (column.format) {
            case column_type::objtype:
                m_buffer += osmium::item_type_to_char(object.type());
                break;
            case column_type::id:
                m_buffer.append(std::to_string(object.id()));
                break;
            case column_type::version:
                m_buffer.append(std::to_string(object.version()));
                break;
            case column_type::deleted:
                m_buffer += object.visible() ? 'f' : 't';
                break;
            case column_type::visible:
                m_buffer += object.visible() ? 't' : 'f';
                break;
            case column_type::changeset:
                m_buffer.append(std::to_string(object.changeset()));
                break;
            case column_type::timestamp_iso:
                m_buffer.append(object.timestamp().to_iso());
                break;
            case column_type::timestamp_sec:
                m_buffer.append(std::to_string(static_cast<uint64_t>(object.timestamp())));
                break;
            case column_type::timestamp_range:
                m_buffer += '[';
                m_buffer.append(object.timestamp().to_iso());
                m_buffer += ',';
                if (next_version_timestamp.valid() && next_version_timestamp >= object.timestamp()) {
                    m_buffer.append(next_version_timestamp.to_iso());
                }
                m_buffer += ')';
                break;
            case column_type::uid:
                m_buffer.append(std::to_string(object.uid()));
                break;
            case column_type::user:
                append_pg_escaped(m_buffer, object.user());
                break;
            case column_type::tags_jsonb:
                /* fallthrough */
            case column_type::tags_json:
                add_tags_json(m_buffer, object.tags());
                break;
            case column_type::lon_real:
                append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lon()); });
                break;
            case column_type::lon_int:
                append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.x()); });
                break;
            case column_type::lat_real:
                append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lat()); });
                break;
            case column_type::lat_int:
                append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.y()); });
                break;
            case column_type::tile:
                append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(quadtile(location)); });
                break;
            case column_type::nodes_array:
                if (object.type() == osmium::item_type::way) {
                    add_way_nodes_array(m_buffer, static_cast<const osmium::Way&>(object).nodes());
                } else {
                    m_buffer += "\\N";
                }
                break;
            case column_type::members_jsonb:
                /* fallthrough */
            case column_type::members_json:
                if (object.type() == osmium::item_type::relation) {
                    add_members_json(m_buffer, static_cast<const osmium::Relation&>(object).members());
                } else {
                    m_buffer += "\\N";
                }
                break;
            case column_type::geometry:
                /* fallthrough */
            case column_type::geometry_point:
                if (object.type() == osmium::item_type::node && static_cast<const osmium::Node&>(object).location()) {
                    m_buffer.append(m_factory.create_point(static_cast<const osmium::Node&>(object)));
                } else {
                    m_buffer += "\\N";
                }
                break;
            case column_type::geometry_linestring:
                if (object.type() == osmium::item_type::way) {
                    try {
                        m_buffer.append(m_factory.create_linestring(static_cast<const osmium::Way&>(object)));
                    } catch (const osmium::geometry_error&) {
                        m_buffer += "\\N";
                    }
                } else {
                    m_buffer += "\\N";
                }
                break;
            case column_type::redaction:
                m_buffer += "\\N";
                break;
            default:
                break;
        }
        m_buffer += '\t';
    }
    m_buffer.back() = '\n';
}

void TagsTable::add_row(const osmium::OSMObject& object, const osmium::Timestamp next_version_timestamp) {
    std::size_t n = 0;
    for (const auto& tag : object.tags()) {
        for (const auto& column : m_columns) {
            switch (column.format) {
                case column_type::objtype:
                    m_buffer += osmium::item_type_to_char(object.type());
                    break;
                case column_type::id:
                    m_buffer.append(std::to_string(object.id()));
                    break;
                case column_type::version:
                    m_buffer.append(std::to_string(object.version()));
                    break;
                case column_type::deleted:
                    m_buffer += object.visible() ? 'f' : 't';
                    break;
                case column_type::visible:
                    m_buffer += object.visible() ? 't' : 'f';
                    break;
                case column_type::changeset:
                    m_buffer.append(std::to_string(object.changeset()));
                    break;
                case column_type::timestamp_iso:
                    m_buffer.append(object.timestamp().to_iso());
                    break;
                case column_type::timestamp_sec:
                    m_buffer.append(std::to_string(static_cast<uint64_t>(object.timestamp())));
                    break;
                case column_type::timestamp_range:
                    m_buffer += '[';
                    m_buffer.append(object.timestamp().to_iso());
                    m_buffer += ',';
                    if (next_version_timestamp.valid() && next_version_timestamp >= object.timestamp()) {
                        m_buffer.append(next_version_timestamp.to_iso());
                    }
                    m_buffer += ')';
                    break;
                case column_type::uid:
                    m_buffer.append(std::to_string(object.uid()));
                    break;
                case column_type::user:
                    append_pg_escaped(m_buffer, object.user());
                    break;
                case column_type::tag_seq:
                    m_buffer.append(std::to_string(n));
                    break;
                case column_type::tag_key:
                    m_buffer.append(tag.key());
                    break;
                case column_type::tag_value:
                    m_buffer.append(tag.value());
                    break;
                case column_type::tag_kv:
                    m_buffer.append(tag.key());
                    m_buffer += '=';
                    m_buffer.append(tag.value());
                    break;
                case column_type::lon_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lon()); });
                    break;
                case column_type::lon_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.x()); });
                    break;
                case column_type::lat_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lat()); });
                    break;
                case column_type::lat_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.y()); });
                    break;
                case column_type::redaction:
                    m_buffer += "\\N";
                    break;
                default:
                    break;
            }
            m_buffer += '\t';
        }
        m_buffer.back() = '\n';
        ++n;
    }
}

void WayNodesTable::add_row(const osmium::OSMObject& object, const osmium::Timestamp next_version_timestamp) {
    assert(object.type() == osmium::item_type::way);
    std::size_t n = 0;
    for (const auto& nr : static_cast<const osmium::Way&>(object).nodes()) {
        for (const auto& column : m_columns) {
            switch (column.format) {
                case column_type::objtype:
                    m_buffer += osmium::item_type_to_char(object.type());
                    break;
                case column_type::id:
                    m_buffer.append(std::to_string(object.id()));
                    break;
                case column_type::version:
                    m_buffer.append(std::to_string(object.version()));
                    break;
                case column_type::deleted:
                    m_buffer += object.visible() ? 'f' : 't';
                    break;
                case column_type::visible:
                    m_buffer += object.visible() ? 't' : 'f';
                    break;
                case column_type::changeset:
                    m_buffer.append(std::to_string(object.changeset()));
                    break;
                case column_type::timestamp_iso:
                    m_buffer.append(object.timestamp().to_iso());
                    break;
                case column_type::timestamp_sec:
                    m_buffer.append(std::to_string(static_cast<uint64_t>(object.timestamp())));
                    break;
                case column_type::timestamp_range:
                    m_buffer += '[';
                    m_buffer.append(object.timestamp().to_iso());
                    m_buffer += ',';
                    if (next_version_timestamp.valid() && next_version_timestamp >= object.timestamp()) {
                        m_buffer.append(next_version_timestamp.to_iso());
                    }
                    m_buffer += ')';
                    break;
                case column_type::uid:
                    m_buffer.append(std::to_string(object.uid()));
                    break;
                case column_type::user:
                    append_pg_escaped(m_buffer, object.user());
                    break;
                case column_type::lon_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lon()); });
                    break;
                case column_type::lon_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.x()); });
                    break;
                case column_type::lat_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lat()); });
                    break;
                case column_type::lat_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.y()); });
                    break;
                case column_type::node_seq:
                    m_buffer.append(std::to_string(n));
                    break;
                case column_type::node_ref:
                    m_buffer.append(std::to_string(nr.ref()));
                    break;
                case column_type::redaction:
                    m_buffer += "\\N";
                    break;
                default:
                    break;
            }
            m_buffer += '\t';
        }
        m_buffer.back() = '\n';
        ++n;
    }
}

void MembersTable::add_row(const osmium::OSMObject& object, const osmium::Timestamp next_version_timestamp) {
    assert(object.type() == osmium::item_type::relation);
    std::size_t n = 0;
    for (const auto& member : static_cast<const osmium::Relation&>(object).members()) {
        for (const auto& column : m_columns) {
            switch (column.format) {
                case column_type::objtype:
                    m_buffer += osmium::item_type_to_char(object.type());
                    break;
                case column_type::id:
                    m_buffer.append(std::to_string(object.id()));
                    break;
                case column_type::version:
                    m_buffer.append(std::to_string(object.version()));
                    break;
                case column_type::deleted:
                    m_buffer += object.visible() ? 'f' : 't';
                    break;
                case column_type::visible:
                    m_buffer += object.visible() ? 't' : 'f';
                    break;
                case column_type::changeset:
                    m_buffer.append(std::to_string(object.changeset()));
                    break;
                case column_type::timestamp_iso:
                    m_buffer.append(object.timestamp().to_iso());
                    break;
                case column_type::timestamp_sec:
                    m_buffer.append(std::to_string(static_cast<uint64_t>(object.timestamp())));
                    break;
                case column_type::timestamp_range:
                    m_buffer += '[';
                    m_buffer.append(object.timestamp().to_iso());
                    m_buffer += ',';
                    if (next_version_timestamp.valid() && next_version_timestamp >= object.timestamp()) {
                        m_buffer.append(next_version_timestamp.to_iso());
                    }
                    m_buffer += ')';
                    break;
                case column_type::uid:
                    m_buffer.append(std::to_string(object.uid()));
                    break;
                case column_type::user:
                    append_pg_escaped(m_buffer, object.user());
                    break;
                case column_type::lon_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lon()); });
                    break;
                case column_type::lon_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.x()); });
                    break;
                case column_type::lat_real:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.lat()); });
                    break;
                case column_type::lat_int:
                    append_coordinate(object, m_buffer, [](osmium::Location location) -> std::string { return std::to_string(location.y()); });
                    break;
                case column_type::member_seq:
                    m_buffer += std::to_string(n);
                    break;
                case column_type::member_type:
                    m_buffer += osmium::item_type_to_char(member.type());
                    break;
                case column_type::member_ref:
                    m_buffer += std::to_string(member.ref());
                    break;
                case column_type::member_role:
                    m_buffer += member.role();
                    break;
                case column_type::redaction:
                    m_buffer += "\\N";
                    break;
                default:
                    break;
            }
            m_buffer += '\t';
        }
        m_buffer.back() = '\n';
        ++n;
    }
}

std::string UsersTable::sql_primary_key() const {
    std::string sql;

    sql += "-- ALTER TABLE \"";
    sql += name();
    sql += "\" ADD PRIMARY KEY(uid);\n";

    return sql;
}

void UsersTable::add_row(const osmium::OSMObject& object, const osmium::Timestamp /*next_version_timestamp*/) {
    if (m_user_ids.get(object.uid())) {
        return;
    }

    m_user_ids.set(object.uid());

    for (const auto column : m_columns) {
        switch (column.format) {
            case column_type::uid:
                m_buffer.append(std::to_string(object.uid()));
                break;
            case column_type::user:
                append_pg_escaped(m_buffer, object.user());
                break;
            default:
                m_buffer += "\\N";
                break;
        }
        m_buffer += '\t';
    }
    m_buffer.back() = '\n';
}

static std::unique_ptr<Table> new_table(const std::string& filename, const stream_config_type& stream_config, const std::string& columns_string) {
    std::unique_ptr<Table> ptr;

    switch (stream_config.stype) {
        case stream_type::objects:
            ptr.reset(new ObjectsTable{filename, stream_config, columns_string});
            break;
        case stream_type::tags:
            ptr.reset(new TagsTable{filename, stream_config, columns_string});
            break;
        case stream_type::way_nodes:
            ptr.reset(new WayNodesTable{filename, stream_config, columns_string});
            break;
        case stream_type::members:
            ptr.reset(new MembersTable{filename, stream_config, columns_string});
            break;
        case stream_type::users:
            ptr.reset(new UsersTable{filename, stream_config, columns_string});
            break;
        default:
            std::abort();
    }

    return ptr;
}

std::unique_ptr<Table> create_table(const Options& opts, const std::string& config_string) {
    const auto srp = split(config_string, '=', "o");
    const std::string filename = srp.first;

    const auto sre = split(srp.second, '%');
    const std::string stream = sre.first;
    std::string column_config_string = sre.second;

    const auto& stream_config = get_stream_config(stream);

    if (column_config_string.empty()) {
        if (opts.with_history) {
            column_config_string = stream_config.with_history;
        } else {
            column_config_string = stream_config.without_history;
        }
    }

    return new_table(filename, stream_config, column_config_string);
}

