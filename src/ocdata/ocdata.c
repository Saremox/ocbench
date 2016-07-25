#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ocdata.h"
#include "debug.h"

/* Internal functions */

ocdataStatus
ocdata_create_tables(ocdataContext* ctx)
{
  int ret = SQLITE_OK;
  char* err_msg;
  char* file_structure =
    "CREATE TABLE IF NOT EXISTS file ("
    "   file_id INTEGER NOT NULL PRIMARY KEY,"
    "   path    VARCHAR NOT NULL,"
    "   size    INTEGER NOT NULL );";
  ret = sqlite3_exec(ctx->db, file_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for file: %s", err_msg);

  char* plugin_structure =
    "CREATE TABLE IF NOT EXISTS plugin ("
    "   plugin_id INTEGER NOT NULL PRIMARY KEY,"
    "   name      VARCHAR NOT NULL );";
  ret = sqlite3_exec(ctx->db, plugin_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for plugin: %s", err_msg);

  char* codec_structure =
    "CREATE TABLE IF NOT EXISTS codec ("
    "   codec_id  INTEGER NOT NULL PRIMARY KEY,"
    "   plugin_id INTEGER NOT NULL,"
    "   name      VARCHAR NOT NULL,"
    "   FOREIGN KEY(plugin_id) REFERENCES plugin(plugin_id));";
  ret = sqlite3_exec(ctx->db, codec_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for codec: %s", err_msg);

  char* option_structure =
    "CREATE TABLE IF NOT EXISTS option ("
    "   option_id INTEGER NOT NULL PRIMARY KEY,"
    "   name      VARCHAR NOT NULL );";
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
    "   time            INTEGER NOT NULL,"
    "   PRIMARY KEY (comp_id, file_id),"
    "   FOREIGN KEY(comp_id) REFERENCES compression(comp_id),"
    "   FOREIGN KEY(file_id) REFERENCES file(file_id));";
  ret = sqlite3_exec(ctx->db, result_structure, 0 ,0, &err_msg);
  check(ret == SQLITE_OK, "failed to create table for result: %s", err_msg);

  return OCDATA_SUCCESS;
error:
  return OCDATA_FAILURE;
}

/* interface functions */

ocdataStatus
ocdata_create_context(ocdataContext** ctx, char* dbfilepath, int64_t flags)
{
  (*ctx) = malloc(sizeof(ocdataContext));
  check((*ctx) > 0, "allocation of ocdataContext failed");

  // if(flags & OCDATA_BACKUP)
  // {
  //   struct stat file_info;
  //   if(stat(dbfilepath,&file_info) == 0)
  //   {
  //     // db File exists
  //     char* bakfile = malloc(strlen(dbfilepath)+5);
  //     sprintf(bakfile,"%s.bak",dbfilepath);
  //     int original  = open(dbfilepath,0,"r+b");
  //     int backup    = open(bakfile,0,"w+b");
  //   }
  // }

  int ret = sqlite3_open(dbfilepath,&(*ctx)->db);
  check(ret == SQLITE_OK, "failed to open sqlite database: %s",
    sqlite3_errmsg((*ctx)->db))

  // Create table structures if they don't exist
  ret = ocdata_create_tables((*ctx));
  check(ret == OCDATA_SUCCESS, "failed to create table structures");

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

}

ocdataStatus
ocdata_add_plugin(ocdataContext* ctx, ocdataPlugin* file)
{

}

ocdataStatus
ocdata_add_comp_option(ocdataContext* ctx, ocdataCompressionOption* file)
{

}

ocdataStatus
ocdata_add_comp_options(ocdataContext* ctx, ocdataCompressionOption** options)
{

}

ocdataStatus
ocdata_add_codec(ocdataContext* ctx, ocdataCodec* file)
{

}

ocdataStatus
ocdata_add_comp(ocdataContext* ctx, ocdataCompresion* file)
{

}

ocdataStatus
ocdata_add_result(ocdataContext* ctx, ocdataResult* file)
{

}

ocdataStatus
ocdata_get_file(ocdataContext* ctx, ocdataFile** res, int64_t file_id)
{

}

ocdataStatus
ocdata_get_plugin(ocdataContext* ctx, ocdataPlugin** res, int64_t plugin_id)
{

}

ocdataStatus
ocdata_get_comp_option(ocdataContext* ctx,ocdataCompressionOption** res,
  int64_t comp_id,int64_t option_id)
{

}

ocdataStatus
ocdata_get_comp_options(ocdataContext* ctx, ocdataCompressionOption*** res,
  int64_t comp_id)
{

}

ocdataStatus
ocdata_get_codec(ocdataContext* ctx, ocdataCodec** res, int64_t codec_id)
{

}

ocdataStatus
ocdata_get_comp(ocdataContext* ctx, ocdataCompresion** res, int64_t comp_id)
{

}

ocdataStatus
ocdata_get_result(ocdataContext* ctx, ocdataResult** res,
  int64_t file_id, int64_t comp_id)
{

}

ocdataStatus
ocdata_destroy_context(ocdataContext **ctx)
{

}
