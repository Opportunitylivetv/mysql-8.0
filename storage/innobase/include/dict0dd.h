/*****************************************************************************

Copyright (c) 2015, 2017, Oracle and/or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/** @file include/dict0dd.h
Data dictionary interface */

#ifndef dict0dd_h
#define dict0dd_h

#include "univ.i"
#include "dict0dict.h"
#include "dict0types.h"
#include "dict0mem.h"
#include <dd/properties.h>
#include "sess0sess.h"

#include "dd/dd.h"
#include "dd/dictionary.h"
#include "dd/cache/dictionary_client.h"
#include "dd/properties.h"
#include "dd/sdi_tablespace.h"	// dd::sdi_tablespace::store
#include "dd/dd_table.h"
#include "dd/dd_schema.h"
#include "dd/types/table.h"
#include "dd/types/index.h"
#include "dd/types/column.h"
#include "dd/types/index_element.h"
#include "dd/types/partition.h"
#include "dd/types/partition_index.h"
#include "dd/types/object_type.h"
#include "dd/types/tablespace.h"
#include "dd/types/tablespace_file.h"
#include "dd_table_share.h"
#include "dd/types/foreign_key.h"
#include "dd/types/foreign_key_element.h"

class THD;
class MDL_ticket;

/** WL#9535 TODO: Change this in 9535, and maybe remove this once
WL#6049 and InnoDB FK worklogs */
/** Max table id for DD table */
static constexpr uint	INNODB_DD_TABLE_ID_MAX = 70;

/** Handler name for InnoDB */
static constexpr char handler_name[] = "InnoDB";

static const char innobase_hton_name[]= "InnoDB";

/** InnoDB private keys for dd::Table */
enum dd_table_keys {
	/** Auto-increment counter */
	DD_TABLE_AUTOINC,
	/** DATA DIRECTORY (static metadata) */
	DD_TABLE_DATA_DIRECTORY,
	/** Dynamic metadata version */
	DD_TABLE_VERSION,
	/** Disacard flag */
	DD_TABLE_DISCARD,
	/** Sentinel */
	DD_TABLE__LAST
};

/** InnoDB private keys for dd::Tablespace */
enum dd_space_keys {
	/** Tablespace flags */
	DD_SPACE_FLAGS,
	/** Tablespace identifier */
	DD_SPACE_ID,
	/** Sentinel */
	DD_SPACE__LAST
};

/** InnoDB implicit tablespace name or prefix, which should be same to
dict_sys_t::file_per_table_name */
static constexpr char reserved_implicit_name[] = "innodb_file_per_table";

/** InnoDB private key strings for dd::Tablespace.
@see dd_space_keys */
const char* const dd_space_key_strings[DD_SPACE__LAST] = {
	"flags",
	"id"
};

/** InnoDB private key strings for dd::Table. @see dd_table_keys */
const char* const dd_table_key_strings[DD_TABLE__LAST] = {
	"autoinc",
	"data_directory",
	"version",
	"discard"
};

/** InnoDB private keys for dd::Index or dd::Partition_index */
enum dd_index_keys {
	/** Index identifier */
	DD_INDEX_ID,
	/** Root page number */
	DD_INDEX_ROOT,
	/** Creating transaction ID */
	DD_INDEX_TRX_ID,
	/** Sentinel */
	DD_INDEX__LAST
};

/** InnoDB private key strings for dd::Index or dd::Partition_index.
@see dd_index_keys */
const char* const dd_index_key_strings[DD_INDEX__LAST] = {
	"id",
	"root",
	"trx_id"
};

/** InnoDB private key strings for dd::Index or dd::Partition_index.
@see dd_index_keys */
extern const char* const	dd_index_key_strings[DD_INDEX__LAST];

/** dd::Partition::options() key for INDEX DIRECTORY */
static const dd::String_type	index_file_name_key("index_file_name");
/** dd::Partition::options() key for DATA DIRECTORY */
static const dd::String_type	data_file_name_key("data_file_name");

#ifdef UNIV_DEBUG

/** Hard-coded data dictionary information */
struct innodb_dd_table_t {
	/** Data dictionary table name */
	const char*	name;
	/** Number of indexes */
	const uint	n_indexes;
};

/** The hard-coded data dictionary tables */
const innodb_dd_table_t innodb_dd_table[] = {
	INNODB_DD_TABLE("dd_properties", 1),
	INNODB_DD_TABLE("character_sets", 3),
	INNODB_DD_TABLE("collations", 3),
	INNODB_DD_TABLE("tablespaces", 2),
	INNODB_DD_TABLE("tablespace_files", 2),
	INNODB_DD_TABLE("catalogs", 2),
	INNODB_DD_TABLE("column_statistics", 3),
	INNODB_DD_TABLE("schemata", 3),
	INNODB_DD_TABLE("st_spatial_reference_systems", 2),
	INNODB_DD_TABLE("tables", 6),
	INNODB_DD_TABLE("view_table_usage", 2),
	INNODB_DD_TABLE("view_routine_usage", 2),
	INNODB_DD_TABLE("columns", 4),
	INNODB_DD_TABLE("indexes", 3),
	INNODB_DD_TABLE("index_column_usage", 3),
	INNODB_DD_TABLE("column_type_elements", 1),
	INNODB_DD_TABLE("foreign_keys", 5),
	INNODB_DD_TABLE("foreign_key_column_usage", 3),
	INNODB_DD_TABLE("table_partitions", 6),
	INNODB_DD_TABLE("table_partition_values", 1),
	INNODB_DD_TABLE("index_partitions", 3),
	INNODB_DD_TABLE("table_stats", 1),
	INNODB_DD_TABLE("index_stats", 1),
	INNODB_DD_TABLE("events", 5),
	INNODB_DD_TABLE("routines", 6),
	INNODB_DD_TABLE("parameters", 3),
	INNODB_DD_TABLE("parameter_type_elements", 1),
	INNODB_DD_TABLE("triggers", 6),
	INNODB_DD_TABLE("innodb_table_stats", 1),
	INNODB_DD_TABLE("innodb_index_stats", 1),
	INNODB_DD_TABLE("innodb_dynamic_metadata", 1)
};

/** Number of hard-coded data dictionary tables */
static constexpr size_t innodb_dd_table_size = UT_ARR_SIZE(innodb_dd_table);

/** @return total number of indexes of all DD Tables. */
uint32_t dd_get_total_indexes_num();

#endif /* UNIV_DEBUG */


/** Determine if a dd::Table is partitioned table
@param[in]	table	dd::Table
@return	True	If partitioned table
@retval	False	non-partitioned table */
inline bool dd_table_is_partitioned(const dd::Table& table)
{
	return(table.partition_type() != dd::Table::PT_NONE);
}

/** Get the first index of a table or partition.
@tparam		Table	dd::Table or dd::Partition
@tparam		Index	dd::Index or dd::Partition_index
@param[in]	table	table containing user columns and indexes
@return	the first index
@retval	NULL	if there are no indexes */
template<typename Table, typename Index>
inline
const Index*
dd_first(const Table* table)
{
	return(*table->indexes().begin());
}

/** Get the first index of a table.
@param[in]	table	table containing user columns and indexes
@return	the first index
@retval	NULL	if there are no indexes */
inline
const dd::Index*
dd_first_index(const dd::Table* table)
{
	ut_ad(table->partitions().empty());
	return(dd_first<dd::Table, dd::Index>(table));
}

/** Get the first index of a partition.
@param[in]	partition	partition or subpartition
@return	the first index
@retval	NULL	if there are no indexes */
inline
const dd::Partition_index*
dd_first_index(const dd::Partition* partition)
{
	return(dd_first<dd::Partition,dd::Partition_index>(partition));
}

/** Determine if a partition is materialized.
@param[in]	part		partition
@return whether the partition is materialized */
inline bool dd_part_is_stored(const dd::Partition* part)
{
	return(part->table().subpartition_type() == dd::Table::ST_NONE
	       || part->level() == 1);
}

/** Get the explicit dd::Tablespace::id of a table.
@param[in]	table	non-partitioned table
@return	the explicit dd::Tablespace::id
@retval	dd::INVALID_OBJECT_ID	if there is no explicit tablespace */
inline
dd::Object_id
dd_get_space_id(const dd::Table& table)
{
	ut_ad(!dd_table_is_partitioned(table));
	return(table.tablespace_id());
}

/** Get the explicit dd::Tablespace::id of a partition.
@param[in]	partition	partition or subpartition
@return	the explicit dd::Tablespace::id
@retval	dd::INVALID_OBJECT_ID	if there is no explicit tablespace */
inline
dd::Object_id
dd_get_space_id(const dd::Partition& partition);

/** Set the AUTO_INCREMENT attribute.
@param[in,out]	se_private_data	dd::Table::se_private_data
@param[in]	autoinc		the auto-increment value */
void
dd_set_autoinc(
	dd::Properties&	se_private_data,
	uint64		autoinc);

/** Copy the engine-private parts of a table definition
when the change does not affect InnoDB. Keep the already set
AUTOINC counter related information if exist
@tparam		Table		dd::Table or dd::Partition
@param[in,out]	new_table	Copy of old table or partition definition
@param[in]	old_table	Old table or partition definition */
template<typename Table>
void
dd_copy_private(
	Table&		new_table,
	const Table&	old_table);

/** Write metadata of a table to dd::Table
@param[in]	dd_space_id	Tablespace id, which server allocates
@param[in,out]	dd_table	dd::Table
@param[in]	table		InnoDB table object */
template<typename Table>
void
dd_write_table(
	dd::Object_id		dd_space_id,
	Table*			dd_table,
	const dict_table_t*	table);

/** Set options of dd::Table according to InnoDB table object
@param[in,out]	dd_table	dd::Table
@param[in]	table		InnoDB table object */
void
dd_set_table_options(
	dd::Table*		dd_table,
	const dict_table_t*	table);

/** Write metadata of a tablespace to dd::Tablespace
@param[in,out]	dd_space	dd::Tablespace
@param[in]	tablespace	InnoDB tablespace object */
void
dd_write_tablespace(
	dd::Tablespace*		dd_space,
	const Tablespace&	tablespace);

/** Add fts doc id column and index to new table
when old table has hidden fts doc id without fulltext index
@param[in,out]	new_table	New dd table
@param[in]	old_table	Old dd table */
void
dd_add_fts_doc_id_index(
	dd::Table&		new_table,
	const dd::Table&	old_table);

/** Find the specified dd::Index or dd::Partition_index in an InnoDB table
@tparam		Index			dd::Index or dd::Partition_index
@param[in]	table			InnoDB table object
@param[in]	dd_index		Index to search
@return	the dict_index_t object related to the index */
template<typename Index>
const dict_index_t*
dd_find_index(
	const dict_table_t*	table,
	Index*			dd_index);

/** Acquire a shared metadata lock.
@param[in,out]	thd	current thread
@param[out]	mdl	metadata lock
@param[in]	db	schema name
@param[in]	table	table name
@retval false if acquired, or trylock timed out
@retval true if failed (my_error() will have been called) */
UNIV_INLINE MY_ATTRIBUTE((warn_unused_result))
bool
dd_mdl_acquire(
	THD*			thd,
	MDL_ticket**		mdl,
	const char*		db,
	const char*		table);

/** Release a metadata lock.
@param[in,out]	thd	current thread
@param[in,out]	mdl	metadata lock */
void
dd_mdl_release(
	THD*		thd,
	MDL_ticket**	mdl);

/** Load foreign key constraint info for the dd::Table object.
@param[out]	m_table		InnoDB table handle
@param[in]	dd_table	Global DD table
@param[in]	col_names	column names, or NULL
@param[in]	ignore_err	DICT_ERR_IGNORE_FK_NOKEY or DICT_ERR_IGNORE_NONE
@param[in]	dict_locked	True if dict_sys->mutex is already held,
				otherwise false
@return DB_SUCESS	if successfully load FK constraint */
dberr_t
dd_table_load_fk_from_dd(
	dict_table_t*		m_table,
	const dd::Table*	dd_table,
	const char**		col_names,
	dict_err_ignore_t	ignore_err,
	bool			dict_locked);

/** Set the AUTO_INCREMENT attribute.
@param[in,out]	se_private_data	dd::Table::se_private_data
@param[in]	autoinc		the auto-increment value */
void dd_set_autoinc(dd::Properties& se_private_data, uint64 autoinc);

/** Load foreign key constraint for the table. Note, it could also open
the foreign table, if this table is referenced by the foreign table
@param[in,out]	client		data dictionary client
@param[in]	tbl_name	Table Name
@param[in]	col_names	column names, or NULL
@param[out]	m_table		InnoDB table handle
@param[in]	dd_table	Global DD table
@param[in]	thd		thread THD
@param[in]	dict_locked	True if dict_sys->mutex is already held,
				otherwise false
@param[in]	check_charsets	whether to check charset compatibility
@param[in,out]	fk_tables	name list for tables that refer to this table
@return DB_SUCESS	if successfully load FK constraint */
dberr_t
dd_table_load_fk(
	dd::cache::Dictionary_client*	client,
	const char*			tbl_name,
	const char**			col_names,
	dict_table_t*			m_table,
	const dd::Table*		dd_table,
	THD*				thd,
	bool				dict_locked,
	bool				check_charsets,
	dict_names_t*			fk_tables);

/** Load foreign key constraint for the table. Note, it could also open
the foreign table, if this table is referenced by the foreign table
@param[in,out]	client		data dictionary client
@param[in]	tbl_name	Table Name
@param[in]	col_names	column names, or NULL
@param[out]	m_table		InnoDB table handle
@param[in]	dd_table	Global DD table
@param[in]	thd		thread THD
@param[in]	check_charsets	whether to check charset compatibility
@param[in]	ignore_err	DICT_ERR_IGNORE_FK_NOKEY or DICT_ERR_IGNORE_NONE
@param[in,out]	fk_tables	name list for tables that refer to this table
@return DB_SUCESS	if successfully load FK constraint */
dberr_t
dd_table_check_for_child(
	dd::cache::Dictionary_client*	client,
	const char*			tbl_name,
	const char**			col_names,
	dict_table_t*			m_table,
	const dd::Table*		dd_table,
	THD*				thd,
	bool				check_charsets,
	dict_err_ignore_t		ignore_err,
	dict_names_t*			fk_tables);

/** Instantiate an InnoDB in-memory table metadata (dict_table_t)
based on a Global DD object.
@param[in,out]	client		data dictionary client
@param[in]	dd_table	Global DD table object
@param[in]	dd_part		Global DD partition or subpartition, or NULL
@param[in]	tbl_name	table name, or NULL if not known
@param[out]	table		InnoDB table (NULL if not found or loadable)
@param[in]	thd		thread THD
@return error code
@retval 0	on success */
int
dd_table_open_on_dd_obj(
	dd::cache::Dictionary_client*	client,
	const dd::Table&		dd_table,
	const dd::Partition*		dd_part,
	const char*			tbl_name,
	dict_table_t*&			table,
	THD*				thd);

/** Open a persistent InnoDB table based on table id.
@param[in]	table_id	table identifier
@param[in,out]	thd		current MySQL connection (for mdl)
@param[in,out]	mdl		metadata lock (*mdl set if table_id was found); mdl=NULL if we are resurrecting table IX locks in recovery
@return table
@retval NULL if the table does not exist or cannot be opened */
dict_table_t*
dd_table_open_on_id(
	table_id_t	table_id,
	THD*		thd,
	MDL_ticket**	mdl,
	bool		dict_locked);

/** Close an internal InnoDB table handle.
@param[in,out]	table	InnoDB table handle
@param[in,out]	thd	current MySQL connection (for mdl)
@param[in,out]	mdl	metadata lock (will be set NULL)
@param[in]	dict_locked	whether we hold dict_sys mutex */
void
dd_table_close(
	dict_table_t*	table,
	THD*		thd,
	MDL_ticket**	mdl,
	bool		dict_locked);

/** Set the discard flag for a dd table.
@param[in,out]	thd	current thread
@param[in]	table	InnoDB table
@param[in]	discard	discard flag
@retval false if fail. */
bool
dd_table_discard_tablespace(
	THD*			thd,
	const dict_table_t*	table,
	dd::Table*		table_def,
	bool			discard);

/** Open an internal handle to a persistent InnoDB table by name.
@param[in,out]	thd		current thread
@param[out]	mdl		metadata lock
@param[in]	name		InnoDB table name
@param[in]	dict_locked	has dict_sys mutex locked
@param[in]	ignore_err	whether to ignore err
@return handle to non-partitioned table
@retval NULL if the table does not exist */
dict_table_t*
dd_table_open_on_name(
	THD*			thd,
	MDL_ticket**		mdl,
	const char*		name,
	bool			dict_locked,
	ulint			ignore_err);

/** Returns a cached table object based on table id.
@param[in]	table_id	table id
@param[in]	dict_locked	TRUE=data dictionary locked
@return table, NULL if does not exist */
UNIV_INLINE
dict_table_t*
dd_table_open_on_id_in_mem(
	table_id_t	table_id,
	bool		dict_locked);

/** Returns a cached table object based on table name.
@param[in]	name		table name
@param[in]	dict_locked	TRUE=data dictionary locked
@return table, NULL if does not exist */
UNIV_INLINE
dict_table_t*
dd_table_open_on_name_in_mem(
	const char*	name,
	ibool		dict_locked);

/** Open or load a table definition based on a Global DD object.
@param[in,out]	client		data dictionary client
@param[in]	table		MySQL table definition
@param[in]	norm_name	Table Name
@param[in]	dd_table	Global DD table or partition object
@param[in]	thd		thread THD
@return ptr to dict_table_t filled, otherwise, nullptr */
template<typename Table>
dict_table_t*
dd_open_table(
	dd::cache::Dictionary_client*	client,
	const TABLE*			table,
	const char*			norm_name,
	const Table*			dd_table,
	THD*				thd);

/** Open foreign tables reference a table.
@param[in,out]	client		data dictionary client
@param[in]	fk_list		foreign key name list
@param[in]	dict_locked	dict_sys mutex is locked or not
@param[in]	thd		thread THD */
void
dd_open_fk_tables(
	dd::cache::Dictionary_client*	client,
	dict_names_t&			fk_list,
	bool				dict_locked,
	THD*				thd);

/** Update filename of dd::Tablespace
@param[in]	dd_space_id	dd tablespace id
@param[in]	new_path	new data file path
@retval false if fail. */
bool
dd_tablespace_update_filename(
	dd::Object_id		dd_space_id,
	const char*		new_path);

/* Create metadata for specified tablespace, acquiring exlcusive MDL first
@param[in,out]	dd_client	data dictionary client
@param[in,out]	thd		THD
@param[in,out]	dd_space_name	dd tablespace name
@param[in]	space		InnoDB tablespace ID
@param[in]	flags		InnoDB tablespace flags
@param[in]	filename	filename of this tablespace
@param[in,out]	dd_space_id	dd_space_id
@retval	false	on success
@retval	true	on failure */
bool
create_dd_tablespace(
	dd::cache::Dictionary_client*	dd_client,
	THD*				thd,
	const char*			dd_space_name,
	space_id_t			space_id,
	ulint				flags,
	const char*			filename,
	dd::Object_id&			dd_space_id);

/** Create metadata for implicit tablespace
@param[in,out]	dd_client	data dictionary client
@param[in,out]	thd		THD
@param[in]	space		InnoDB tablespace ID
@param[in]	filename	tablespace filename
@param[in,out]	dd_space_id	dd tablespace id
@retval	false	on success
@retval	true	on failure */
bool
dd_create_implicit_tablespace(
	dd::cache::Dictionary_client*	dd_client,
	THD*				thd,
	space_id_t			space,
	const char*			filename,
	dd::Object_id&			dd_space_id);

/** Obtain the private handler of InnoDB session specific data.
@param[in,out]	thd	MySQL thread handler.
@return reference to private handler */
MY_ATTRIBUTE((warn_unused_result))
innodb_session_t*&
thd_to_innodb_session(
	THD*	thd);

/** Parse a table file name into table name and database name
@param[in]	tbl_name	table name including database and table name
@param[in,out]	dd_db_name	database name buffer to be filled
@param[in,out]	dd_tbl_name	table name buffer to be filled
@param[in,out]	dd_ptn_name	partition name to be filled if not NULL
@return	true if table name is parsed properly, false if the table name
is invalid */
UNIV_INLINE
bool
dd_parse_tbl_name(
	const char*	tbl_name,
	char*		dd_db_name,
	char*		dd_tbl_name,
	char*		dd_ptn_name);

/** Look up a column in a table using the system_charset_info collation.
@param[in]	dd_table	data dictionary table
@param[in]	name		column name
@return the column
@retval nullptr if not found */
UNIV_INLINE
const dd::Column*
dd_find_column(
	const dd::Table*	dd_table,
	const char*		name);

/** Add a hidden column when creating a table.
@param[in,out]	dd_table	table containing user columns and indexes
@param[in]	name		hidden column name
@param[in]	length		length of the column, in bytes
@param[in]	type		column type
@return the added column, or NULL if there already was a column by that name */
UNIV_INLINE
dd::Column*
dd_add_hidden_column(
	dd::Table*		dd_table,
	const char*		name,
	uint			length,
	dd::enum_column_types	type);

/** Add a hidden index element at the end.
@param[in,out]	index	created index metadata
@param[in]	column	column of the index */
UNIV_INLINE
void
dd_add_hidden_element(dd::Index* index, const dd::Column* column);

/** Initialize a hidden unique B-tree index.
@param[in,out]	index	created index metadata
@param[in]	name	name of the index
@param[in]	column	column of the index
@return the initialized index */
UNIV_INLINE
dd::Index*
dd_set_hidden_unique_index(
	dd::Index*		index,
	const char*		name,
	const dd::Column*	column);

/** Check whether there exist a column named as "FTS_DOC_ID", which is
reserved for InnoDB FTS Doc ID
@param[in]	thd		MySQL thread handle
@param[in]	form		information on table
				columns and indexes
@param[out]	doc_id_col	Doc ID column number if
				there exist a FTS_DOC_ID column,
				ULINT_UNDEFINED if column is of the
				wrong type/name/size
@return true if there exist a "FTS_DOC_ID" column */
UNIV_INLINE
bool
create_table_check_doc_id_col(
	THD*		thd,
	const TABLE*	form,
	ulint*		doc_id_col);

/** Return a display name for the row format
@param[in]	row_format	Row Format
@return row format name */
UNIV_INLINE
const char*
get_row_format_name(
	enum row_type row_format);

#ifdef UNIV_DEBUG

/** Get the file name of a tablespace.
@param[in]	dd_space	Tablespace metadata
@return file name */
inline
const char*
dd_tablespace_get_filename(const dd::Tablespace* dd_space)
{
	ut_ad(dd_space->id() != dd::INVALID_OBJECT_ID);
	ut_ad(dd_space->files().size() == 1);
	return((*dd_space->files().begin())->filename().c_str());
}

#endif /* UNIV_DEBUG */

/** WL#9536 TODO: Define this function in debug mode only.
And just assert it on opening tables, instead of setting it as corrupted */
/** Check if the InnoDB table is consistent with dd::Table
@param[in]	table			InnoDB table
@param[in]	dd_table		dd::Table or dd::Partition
@return	true	if match
@retval	false	if not match */
template<typename Table>
bool
dd_table_match(
	const dict_table_t*	table,
	const Table*		dd_table);

/** Create dd table for fts aux index table
@param[in]	parent_table	parent table of fts table
@param[in,out]	table		fts table
@param[in]	charset		fts index charset
@return true on success, false on failure */
bool
dd_create_fts_index_table(
	const dict_table_t*	parent_table,
	dict_table_t*		fts_table,
	const CHARSET_INFO*	charset);

/** Create dd table for fts aux common table
@param[in]	parent_table	parent table of fts table
@param[in,out]	table		fts table
@param[in]	is_config	flag whether it's fts aux configure table
@return true on success, false on failure */
bool
dd_create_fts_common_table(
	const dict_table_t*	parent_talbe,
	dict_table_t*		table,
	bool			is_config);

/** Drop dd table & tablespace for fts aux table
@param[in]	name		table name
@param[in]	file_per_table	flag whether use file per table
@return true on success, false on failure. */
bool
dd_drop_fts_table(
	const char*	name,
	bool		file_per_table);

/** Rename dd table & tablespace files for fts aux table
@param[in]	table		dict table
@param[in]	old_name	old innodb table name
@return true on success, false on failure. */
bool
dd_rename_fts_table(
	const dict_table_t*	table,
	const char*		old_name);

#include "dict0dd.ic"
#endif
