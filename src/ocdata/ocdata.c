/**
 * This File is part of openCompressBench
 * Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
 *
 * openCompressBench is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation,
 * version 2 of the License.
 *
 * openCompressBench is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openCompressBench.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-2.0 <https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html>
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ocdata.h"
#include "debug.h"

// Pointer lists

List __files         = {0,0,0,(ocutilsListFreeFunction)ocdata_free_file};
List __plugins       = {0,0,0,(ocutilsListFreeFunction)ocdata_free_plugin};
List __codecs        = {0,0,0,(ocutilsListFreeFunction)ocdata_free_codec};
List __options       = {0,0,0,NULL};
List __comp_options  = {0,0,0,(ocutilsListFreeFunction)ocdata_free_comp_option};
List __comp          = {0,0,0,(ocutilsListFreeFunction)ocdata_free_comp};
List __results       = {0,0,0,(ocutilsListFreeFunction)ocdata_free_result};

static List* files         = &__files;
static List* plugins       = &__plugins;
static List* codecs        = &__codecs;
static List* options       = &__options;
static List* comp_options  = &__comp_options;
static List* comp          = &__comp;
static List* results       = &__results;

#define RESET_STATEMENT(db, stmt, ret) ret = sqlite3_reset(stmt); \
check(ret == SQLITE_OK,"cannot reset prepared statement: %s", \
  sqlite3_errmsg(db)); \
ret = sqlite3_clear_bindings(stmt); \
check(ret == SQLITE_OK,"cannot clear bindings of prepared statement: %s", \
  sqlite3_errmsg(db));

/* Internal functions */

ocdataStatus
ocdata_create_tables(ocdataContext* ctx)
{
  int ret = SQLITE_OK;
  char* err_msg;
  char* file_structure =
    "CREATE TABLE IF NOT EXISTS file ("
    "   file_id INTEGER NOT NULL PRIMARY KEY,"
    "   path    VARCHAR NOT NULL UNIQUE,"
    "   size    INTEGER NOT NULL );";
  ret = sqlite3_exec(ctx->db, file_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for file: %s", err_msg);

  char* plugin_structure =
    "CREATE TABLE IF NOT EXISTS plugin ("
    "   plugin_id INTEGER NOT NULL PRIMARY KEY,"
    "   name      VARCHAR NOT NULL UNIQUE );";
  ret = sqlite3_exec(ctx->db, plugin_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for plugin: %s", err_msg);

  char* codec_structure =
    "CREATE TABLE IF NOT EXISTS codec ("
    "   codec_id  INTEGER NOT NULL PRIMARY KEY,"
    "   plugin_id INTEGER NOT NULL,"
    "   name      VARCHAR NOT NULL,"
    "   UNIQUE(plugin_id,name) "
    "   FOREIGN KEY(plugin_id) REFERENCES plugin(plugin_id));";
  ret = sqlite3_exec(ctx->db, codec_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for codec: %s", err_msg);

  char* option_structure =
    "CREATE TABLE IF NOT EXISTS option ("
    "   option_id INTEGER NOT NULL PRIMARY KEY,"
    "   name      VARCHAR NOT NULL UNIQUE);";
  ret = sqlite3_exec(ctx->db, option_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for option: %s", err_msg);

  char* compression_structure =
    "CREATE TABLE IF NOT EXISTS compression ("
    "   comp_id   INTEGER NOT NULL PRIMARY KEY,"
    "   codec_id  INTEGER NOT NULL,"
    "   FOREIGN KEY(codec_id) REFERENCES codec(codec_id));";
  ret = sqlite3_exec(ctx->db, compression_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK,
    "failed to create table for compression: %s", err_msg);

  char* compression_option_structure =
    "CREATE TABLE IF NOT EXISTS compression_option ("
    "   comp_id   INTEGER NOT NULL,"
    "   option_id INTEGER NOT NULL,"
    "   value     VARCHAR NOT NULL,"
    "   PRIMARY KEY (comp_id, option_id),"
    "   FOREIGN KEY(comp_id) REFERENCES compression(comp_id),"
    "   FOREIGN KEY(option_id) REFERENCES option(option_id));";
  ret = sqlite3_exec(ctx->db, compression_option_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK,
    "failed to create table for compression_option: %s", err_msg);

  char* result_structure =
    "CREATE TABLE IF NOT EXISTS result ("
    "   comp_id         INTEGER NOT NULL,"
    "   file_id         INTEGER NOT NULL,"
    "   compressed_size INTEGER NOT NULL,"
    "   compressed_time INTEGER NOT NULL,"
    "   decompressed_time INTEGER NOT NULL,"
    "   PRIMARY KEY (comp_id, file_id),"
    "   FOREIGN KEY(comp_id) REFERENCES compression(comp_id),"
    "   FOREIGN KEY(file_id) REFERENCES file(file_id));";
  ret = sqlite3_exec(ctx->db, result_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for result: %s", err_msg);

  char* compression_option_codec_view_structure =
  "CREATE VIEW IF NOT EXISTS compression_option_codec_view AS"
  "   SELECT a.comp_id as comp_id, a.codec_id as codec_id, "
  "     b.name as OP_name, c.value as OP_Value"
  "   FROM compression as a, compression_option as c, option as b"
  "   WHERE a.comp_id = c.comp_id AND c.option_id = b.option_id;";
  ret = sqlite3_exec(ctx->db, compression_option_codec_view_structure,
    0 ,0, &err_msg);
  check(ret == SQLITE_OK,
    "failed to create table for compression_option_codec_view_structure: %s",
    err_msg);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_prepare_statements(ocdataContext* ctx)
{
  int ret = SQLITE_OK;
  char* sql;
  sql = "INSERT INTO file(path,size) VALUES (? , ?)";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->file_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO plugin(name) VALUES (?)";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->plugin_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO codec(plugin_id, name) VALUES ( ? , ? )";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->codec_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO compression(codec_id) VALUES (?)";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->compression_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO option(name) VALUES (?)";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->option_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO compression_option(comp_id, option_id, value) "
        "   VALUES ( ? , ? , ?);";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->compression_option_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  sql = "INSERT INTO result(comp_id, file_id, compressed_size, compressed_time,"
        "decompressed_time)"
        "   VALUES ( ? , ? , ? , ? , ? );";
  ret = sqlite3_prepare_v2(ctx->db, sql, -1, &ctx->result_add, 0);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_get_id(ocdataContext* ctx, char* table,
  char* field, char* value, int64_t* id_ptr)
{
  return ocdata_get_id_with_condition(ctx,table,field,value,id_ptr,NULL);
}

ocdataStatus
ocdata_get_id_with_condition(ocdataContext* ctx, char* table,
  char* field, char* value, int64_t* id_ptr, char* extra_condition)
{  
  int           foundids  = 0;
  int           ret       = SQLITE_OK;
  sqlite3_stmt* querry    = NULL;
  char*         fmtquerry;
  if(extra_condition == NULL)
  {
	fmtquerry = "SELECT rowid FROM %s WHERE %s = \"%s\" ;";
	extra_condition = "";
  }
  else
    fmtquerry = "SELECT rowid FROM %s WHERE %s = \"%s\" AND %s ;";
  int           maxlen    = strlen(fmtquerry) +
                            strlen(table) +
                            strlen(field) +
                            strlen(value) +
                            strlen(extra_condition);

  char* sql = calloc(maxlen,1);
  check(sql > 0, "cannot allocate string buffer");
  sprintf(sql,fmtquerry,table,field,value,extra_condition);
  debug("%s",sql);
  ret = sqlite3_prepare_v2(ctx->db, sql , -1 , &querry, NULL);
  check(ret == SQLITE_OK, "failed to prepare statement: %s",
    sqlite3_errmsg(ctx->db));
  ret = sqlite3_step(querry);
  while (ret != SQLITE_DONE && ret != SQLITE_OK) {
    foundids++;
    int cols = sqlite3_column_count(querry);
    if(cols ==1)
      (*id_ptr) = sqlite3_column_int(querry, 0);
    ret = sqlite3_step(querry);
  }

  sqlite3_finalize(querry);
  free(sql);

  if (foundids > 1)
    return OCDATA_MORE_THAN_ONE;
  else if(foundids == 1)
    return OCDATA_SUCCESS;
  else
    return OCDATA_NOT_FOUND;
error:
  return OCDATA_FAILURE;
}

/* interface functions */

void ocdata_free_file         (ocdataFile*              file)
{
  if(file->path != NULL)
    free(file->path);
  free(file);
}

void ocdata_free_plugin       (ocdataPlugin*            plugin)
{
  if(plugin->name != NULL)
    free(plugin->name);
  free(plugin);
}

void ocdata_free_comp_option  (ocdataCompressionOption* comp_option)
{
  free(comp_option);
}

void ocdata_free_codec        (ocdataCodec*             codec)
{
  if(codec->name != NULL)
    free(codec->name);
  free(codec);
}

void ocdata_free_comp         (ocdataCompresion*        comp)
{
  free(comp);
}

void ocdata_free_result       (ocdataResult*            result)
{
  free(result);
}

ocdataFile*
ocdata_new_file(int64_t id, const char* path, size_t size)
{
  ocdataFile* tmp = malloc(sizeof(ocdataFile));
  tmp->file_id    = id;
  tmp->size       = size;
  if (path == NULL)
  {
    tmp->path       = NULL;
  }
  else
  {
    tmp->path       = malloc(strlen(path)+1);
                      strcpy(tmp->path,path);
  }

  ocutils_list_add(files,tmp);

  return tmp;
}

ocdataPlugin*
ocdata_new_plugin(int64_t id, const char* name)
{
  ocdataPlugin* tmp = malloc(sizeof(ocdataPlugin));
  tmp->plugin_id    = id;
  if (name == NULL)
  {
    tmp->name       = NULL;
  }
  else
  {
    tmp->name         = malloc(strlen(name)+1);
                        strcpy(tmp->name,name);
  }

  ocutils_list_add(plugins,tmp);

  return tmp;
}

ocdataCodec*
ocdata_new_codec(int64_t id, ocdataPlugin* plugin, const char* name)
{
  ocdataCodec*   tmp = malloc(sizeof(ocdataCodec));
  tmp->codec_id      = id;
  tmp->plugin_id     = plugin;
  if (name == NULL)
  {
    tmp->name       = NULL;
  }
  else
  {
    tmp->name          = malloc(strlen(name)+1);
                         strcpy(tmp->name,name);
  }

  ocutils_list_add(codecs,tmp);

  return tmp;
}

ocdataCompressionOption*
ocdata_new_comp_option(ocdataCompresion* comp, const char* option_name,
  const char* option_value
)
{
  ocdataCompressionOption* tmp = malloc(sizeof(ocdataCompressionOption));
  tmp->comp_id              = comp;
  tmp->value                = malloc(strlen(option_value)+1);
                              strcpy(tmp->value,option_value);
  tmp->option_id            = malloc(sizeof(ocdataOption));
  tmp->option_id->name      = malloc(strlen(option_name)+1);
                              strcpy(tmp->option_id->name, option_name);
  tmp->option_id->option_id = -1;
  return tmp;
}

ocdataCompresion*
ocdata_new_comp(int64_t id, ocdataCodec* codec_id, List* options)
{
  ocdataCompresion* tmp = malloc(sizeof(ocdataCompresion));
  tmp->comp_id  = id;
  tmp->codec_id = codec_id;
  tmp->options  = options;

  ocutils_list_add(comp,tmp);

  return tmp;
}

ocdataResult*
ocdata_new_result(ocdataCompresion* comp, ocdataFile* file, size_t compressed,
  int64_t compressed_time, int64_t decompressed_time)
{
  ocdataResult* tmp     = malloc(sizeof(ocdataResult));
  tmp->comp_id          = comp;
  tmp->file_id          = file;
  tmp->compressed_size  = compressed;
  tmp->compressed_time  = compressed_time;
  tmp->decompressed_time= decompressed_time;

  ocutils_list_add(results,tmp);

  return tmp;
}

ocdataStatus
ocdata_create_context(ocdataContext** ctx, char* dbfilepath, int64_t flags)
{
  (*ctx) = malloc(sizeof(ocdataContext));
  check((*ctx) > 0, "allocation of ocdataContext failed");
  int ret = sqlite3_open(dbfilepath,&(*ctx)->db);
  check(ret == SQLITE_OK, "failed to open sqlite database: %s",
    sqlite3_errmsg((*ctx)->db))

  // Create table structures if they don't exist
  ret = ocdata_create_tables((*ctx));
  check(ret == OCDATA_SUCCESS, "failed to create table structures");

  ocdata_prepare_statements((*ctx));

  return OCDATA_SUCCESS;
error:
  sqlite3_close((*ctx)->db);
  if((*ctx) > 0)
    free((*ctx));
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_file(ocdataContext* ctx, ocdataFile* file)
{
  int ret = SQLITE_OK;
  check(file->path > 0 && strlen(file->path) > 0,
    "not a valid file path: %s",file->path);
  check(file->size >= 0, "less than 0 bytes invalid file size.");

  // check if entry allready exists
  ocdataStatus st = ocdata_get_id(ctx, "file", "path",
    file->path, &file->file_id);
  if(st == OCDATA_SUCCESS)
    return OCDATA_SUCCESS;

  sqlite3_bind_text(ctx->file_add, 1, file->path,
      strlen(file->path), SQLITE_STATIC);
  sqlite3_bind_int(ctx->file_add, 2, file->size);
  ret = sqlite3_step(ctx->file_add);
  check(ret == SQLITE_DONE,"cannot execute prepared statement: %s",
    sqlite3_errmsg(ctx->db));
  file->file_id = sqlite3_last_insert_rowid(ctx->db);
  RESET_STATEMENT(ctx->db, ctx->file_add, ret);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_plugin(ocdataContext* ctx, ocdataPlugin* plugin)
{
  int ret = SQLITE_OK;
  check(plugin->name > 0 && strlen(plugin->name) > 0,
    "not a valid plugin name: %s",plugin->name);

  // check if entry allready exists
  ocdataStatus st = ocdata_get_id(ctx, "plugin", "name",
    plugin->name, &plugin->plugin_id);
  if(st == OCDATA_SUCCESS)
    return OCDATA_SUCCESS;

  sqlite3_bind_text(ctx->plugin_add, 1, plugin->name,
      strlen(plugin->name), SQLITE_STATIC);
  ret = sqlite3_step(ctx->plugin_add);
  check(ret == SQLITE_DONE,"cannot execute prepared statement: %s",
    sqlite3_errmsg(ctx->db));
  plugin->plugin_id = sqlite3_last_insert_rowid(ctx->db);
  RESET_STATEMENT(ctx->db, ctx->plugin_add, ret);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_option(ocdataContext* ctx, ocdataOption* option)
{
  int ret = SQLITE_OK;
  check(option->name > 0 && strlen(option->name) > 0,
    "not a valid option name: %s",option->name);

  // check if entry allready exists
  ocdataStatus st = ocdata_get_id(ctx, "option", "name",
    option->name, &option->option_id);
  if(st == OCDATA_SUCCESS)
    return OCDATA_SUCCESS;

  sqlite3_bind_text(ctx->option_add, 1, option->name,
      strlen(option->name), SQLITE_STATIC);
  ret = sqlite3_step(ctx->option_add);
  check(ret == SQLITE_DONE,"cannot execute prepared statement: %s",
    sqlite3_errmsg(ctx->db));
  option->option_id = sqlite3_last_insert_rowid(ctx->db);
  RESET_STATEMENT(ctx->db, ctx->option_add, ret);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_comp_option(ocdataContext* ctx, ocdataCompressionOption* compOption)
{
  int ret = SQLITE_OK;
  check(compOption->value > 0, "invalid compOption");
  check(compOption->option_id > 0, "invalid option_id pointer");
  check(compOption->comp_id > 0, "invalid comp_id pointer");

  // We require a valid compression id to fulfil the foreign key in sqlite
  check(compOption->comp_id->comp_id > 0,
    "ocdataCompressionOption does not point to valid compression");

  // Check and get Option id
  if(compOption->option_id->option_id <= 0)
    check(ocdata_add_option(ctx,compOption->option_id) == OCDATA_SUCCESS,
      "failed to insert new option");

  char* sql = "SELECT rowid FROM compression_option "
              " WHERE option_id = ? "
              " AND comp_id = ? ;";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, 0);
  sqlite3_bind_int(stmt,1,compOption->option_id->option_id);
  sqlite3_bind_int(stmt,2,compOption->comp_id->comp_id);
  ret = sqlite3_step(stmt);
  if(ret == SQLITE_DONE)
  {
    sqlite3_bind_int(ctx->compression_option_add,1,
      compOption->comp_id->comp_id);
    sqlite3_bind_int(ctx->compression_option_add,2,
      compOption->option_id->option_id);
    sqlite3_bind_text(ctx->compression_option_add, 3, compOption->value,
      strlen(compOption->value), SQLITE_STATIC);
    ret = sqlite3_step(ctx->compression_option_add);
    check(ret == SQLITE_DONE,"failed to insert compression_option:%s",
      sqlite3_errmsg(ctx->db));
    RESET_STATEMENT(ctx->db, ctx->compression_option_add, ret);
  }
  sqlite3_finalize(stmt);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_comp_options(ocdataContext* ctx, List* options)
{
  ocutils_list_foreach_f(options, cur, ocdataCompressionOption*)
  {
    ocdata_add_comp_option(ctx,cur);
  }
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_add_codec(ocdataContext* ctx, ocdataCodec* codec)
{
  int ret = SQLITE_OK;
  check(codec->name > 0, "invalid codec name");
  check(codec->plugin_id > 0, "invalid plugin pointer");
  
  // get plugin id or instert it into db
  ret = ocdata_add_plugin(ctx,codec->plugin_id);
  check(ret == OCDATA_SUCCESS, "failed to get plugin id or insert plugin");
  check(codec->plugin_id->plugin_id > 0, "invalid plugin id.");

  debug("plugin id is %ld" ,codec->plugin_id->plugin_id);

  char* fmtstring = "plugin_id = %ld";
  int	fmtlen	  = snprintf(NULL,0,fmtstring,codec->plugin_id->plugin_id)+1;
  char* condition = malloc(fmtlen);
  snprintf(condition,fmtlen,fmtstring,codec->plugin_id->plugin_id);

  // check if entry allready exists
  ocdataStatus st = ocdata_get_id_with_condition(ctx, "codec", "name",
    codec->name, &codec->codec_id,condition);
  free(condition);
  if(st == OCDATA_SUCCESS)
    return OCDATA_SUCCESS;

  sqlite3_bind_int(ctx->codec_add, 1, codec->plugin_id->plugin_id);
  sqlite3_bind_text(ctx->codec_add, 2, codec->name,
    strlen(codec->name), SQLITE_STATIC);
  ret = sqlite3_step(ctx->codec_add);
  codec->codec_id = sqlite3_last_insert_rowid(ctx->db);

  RESET_STATEMENT(ctx->db, ctx->codec_add, ret);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_get_comp_id(ocdataContext* ctx, ocdataCompresion* compression)
{
  sqlite3_stmt* querry;
  int           ret       =   SQLITE_OK;
  char *        fmtquerry =   "SELECT comp_id, count(comp_id) as foundops"
                              "  FROM compression_option_codec_view"
                              "  WHERE codec_id = %d"
                              "  AND (%s)"
                              "  GROUP BY comp_id"
                              "  ORDER BY foundops DESC;";
  char *        fmtAnd    =   "(OP_value = \"%s\" AND OP_name = \"%s\")";
  char *        optSearch =   calloc(1,1);
  ocutils_list_foreach_f(compression->options, curOp, ocdataCompressionOption*)
  {
    size_t tmpsize = snprintf(NULL,0,fmtAnd,curOp->value,curOp->option_id->name);
    char*  tmpstr  = calloc(tmpsize+1,1);
    sprintf(tmpstr,fmtAnd,curOp->value,curOp->option_id->name);
    char* ret = realloc(optSearch, strlen(optSearch) + strlen(tmpstr)+6);
    if(!(ret > 0))
    {
        log_err("failed to reallocate string buffer. FATAL");
        exit(EXIT_FAILURE);
    }
    strcat(ret,tmpstr);
    if(curOp_node->next > 0)
    {
      strcat(ret," OR ");
    }
    optSearch = ret;
    free(tmpstr);
  }
  size_t tmpsize = snprintf(NULL,0,fmtquerry,
    compression->codec_id->codec_id,
    optSearch);
  char*  tmpstr  = calloc(tmpsize+1,1);
  sprintf(tmpstr,fmtquerry,
    compression->codec_id->codec_id,
    optSearch);

  ret = sqlite3_prepare_v2(ctx->db, tmpstr , -1 , &querry, NULL);
  // Early freeing of resources which are no longer needed
  free(optSearch);
  free(tmpstr);

  ret = sqlite3_step(querry);
  if(ret != SQLITE_DONE && ret != SQLITE_OK)
  {
    int count = sqlite3_column_int(querry, 1);
    if(count == compression->options->items)
      compression->comp_id = sqlite3_column_int(querry, 0);
  }
  sqlite3_finalize(querry);

  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_add_comp(ocdataContext* ctx, ocdataCompresion* compression)
{
  int ret = SQLITE_OK;
  check(compression > 0, "invalid compression pointer");
  check(compression->codec_id > 0, "invalid codec pointer");

  ocdata_add_codec(ctx,compression->codec_id);

  // check if the given compression is allready in our database
  ret = ocdata_get_comp_id(ctx,compression);
  if(compression->comp_id >0)
    return OCDATA_SUCCESS;

  sqlite3_bind_int(ctx->compression_add, 1, compression->codec_id->codec_id);
  sqlite3_step(ctx->compression_add);
  compression->comp_id = sqlite3_last_insert_rowid(ctx->db);
  RESET_STATEMENT(ctx->db, ctx->compression_add, ret);

  ocutils_list_foreach_f(compression->options, cur, ocdataCompressionOption*)
  {
    cur->comp_id = compression;
  }

  // Insert options into database
  ocdata_add_comp_options(ctx,compression->options);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_add_result(ocdataContext* ctx, ocdataResult* result)
{
  int ret = SQLITE_OK;
  check(result > 0, "invalid result pointer");
  check(result->comp_id > 0, "invalid compression pointer");
  check(result->file_id > 0, "invalid file pointer");

  check(ocdata_add_comp(ctx,result->comp_id) == OCDATA_SUCCESS,
    "failed to insert compression");
  check(ocdata_add_file(ctx,result->file_id) == OCDATA_SUCCESS,
    "failed to insert files");

  sqlite3_bind_int(ctx->result_add, 1, result->comp_id->comp_id);
  sqlite3_bind_int(ctx->result_add, 2, result->file_id->file_id);
  sqlite3_bind_int(ctx->result_add, 3, result->compressed_size);
  sqlite3_bind_int(ctx->result_add, 4, result->compressed_time);
  sqlite3_bind_int(ctx->result_add, 5, result->decompressed_time);
  ret = sqlite3_step(ctx->result_add);

  RESET_STATEMENT(ctx->db, ctx->result_add, ret);

  // TODO what happens if data set allready is in db? sqlite3 will complain
  // of unmet constraints since comp_id and file_id are the PRIMARY KEY

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

ocdataStatus
ocdata_get_file(ocdataContext* ctx, ocdataFile** res, int64_t file_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_plugin(ocdataContext* ctx, ocdataPlugin** res, int64_t plugin_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_comp_option(ocdataContext* ctx,ocdataCompressionOption** res,
  int64_t comp_id,int64_t option_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_comp_options(ocdataContext* ctx, List** options,
  int64_t comp_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_codec(ocdataContext* ctx, ocdataCodec** res, int64_t codec_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_comp(ocdataContext* ctx, ocdataCompresion** res, int64_t comp_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_get_result(ocdataContext* ctx, ocdataResult** res,
  int64_t file_id, int64_t comp_id)
{
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_destroy_context(ocdataContext **ctx)
{
  sqlite3_finalize((*ctx)->file_add);
  sqlite3_finalize((*ctx)->plugin_add);
  sqlite3_finalize((*ctx)->codec_add);
  sqlite3_finalize((*ctx)->option_add);
  sqlite3_finalize((*ctx)->compression_add);
  sqlite3_finalize((*ctx)->compression_option_add);
  sqlite3_finalize((*ctx)->result_add);

  sqlite3_close((*ctx)->db);
  free((*ctx));
  return OCDATA_SUCCESS;
}

ocdataStatus
ocdata_garbage_collect()
{
  ocutils_list_clear(files);
  ocutils_list_clear(plugins);
  ocutils_list_clear(codecs);
  ocutils_list_clear(options);
  ocutils_list_clear(comp_options);
  ocutils_list_clear(comp);
  ocutils_list_clear(results);
  return OCDATA_SUCCESS;
}
