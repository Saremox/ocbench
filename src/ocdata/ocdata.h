#include <sqlite3.h>
#include <stdint.h>
#include "ocutils/list.h"

#ifndef OCDATA_H
#define OCDATA_H

struct __ocdataContext;
struct __ocdataFile;
struct __ocdataOption;
struct __ocdataCompressionOption;
struct __ocdataPlugin;
struct __ocdataCodec;
struct __ocdataCompresion;
struct __ocdataResult;

typedef struct __ocdataContext            ocdataContext;
typedef struct __ocdataFile               ocdataFile;
typedef struct __ocdataOption             ocdataOption;
typedef struct __ocdataCompressionOption  ocdataCompressionOption;
typedef struct __ocdataPlugin             ocdataPlugin;
typedef struct __ocdataCodec              ocdataCodec;
typedef struct __ocdataCompresion         ocdataCompresion;
typedef struct __ocdataResult             ocdataResult;

struct __ocdataContext {
  sqlite3 *db;
  sqlite3_stmt* file_add;
  sqlite3_stmt* plugin_add;
  sqlite3_stmt* codec_add;
  sqlite3_stmt* option_add;
  sqlite3_stmt* compression_add;
  sqlite3_stmt* compression_option_add;
  sqlite3_stmt* result_add;
};

struct __ocdataFile {
  int64_t file_id;
  char*   path;
  size_t  size;
};

struct __ocdataOption {
  int64_t option_id;
  char*   name;
};

struct __ocdataCompressionOption {
  ocdataCompresion* comp_id;
  ocdataOption*     option_id;
  char*             value;
};

struct __ocdataPlugin {
  int64_t plugin_id;
  char*   name;
};

struct __ocdataCodec {
  int64_t       codec_id;
  ocdataPlugin* plugin_id;
  char*         name;
};

struct __ocdataCompresion {
  int64_t       comp_id;
  ocdataCodec*  codec_id;
  List*         options;
};

struct __ocdataResult{
  ocdataCompresion* comp_id;
  ocdataFile*       file_id;
  size_t            compressed_size;
  int64_t           time_needed;
};

typedef enum {
  OCDATA_FAILURE = -1,
  OCDATA_SUCCESS,
  OCDATA_MORE_THAN_ONE,
  OCDATA_NOT_FOUND
} ocdataStatus;

#define OCDATA_OVERWRITE 1<<0

ocdataStatus
ocdata_create_context(
  ocdataContext** ctx,
  char* dbfilepath,
  int64_t flags
);

ocdataStatus
ocdata_get_id(
  ocdataContext* ctx,
  char* table,
  char* field,
  char* value,
  int64_t* id_ptr
);

void ocdata_free_file         (ocdataFile*              file);
void ocdata_free_plugin       (ocdataPlugin*            plugin);
void ocdata_free_codec        (ocdataCodec*             codec);
void ocdata_free_comp_option  (ocdataCompressionOption* comp_option);
void ocdata_free_comp         (ocdataCompresion*        comp);
void ocdata_free_result       (ocdataResult*            result);

ocdataFile*
ocdata_new_file(
  int64_t id,
  const char* path,
  size_t size
);

ocdataPlugin*
ocdata_new_plugin(
  int64_t id,
  const char* name
);

ocdataCodec*
ocdata_new_codec(
  int64_t id,
  ocdataPlugin* plugin,
  const char* name
);

ocdataCompressionOption*
ocdata_new_comp_option(
  ocdataCompresion* comp,
  const char* option_name,
  const char* option_value
);

ocdataCompresion*
ocdata_new_comp(
  int64_t id,
  ocdataCodec* codec_id,
  List* options
);

ocdataResult*
ocdata_new_result(
  ocdataCompresion* comp,
  ocdataFile* file,
  size_t compressed,
  int64_t time_needed
);

ocdataStatus
ocdata_add_file(
  ocdataContext* ctx,
  ocdataFile* file
);

ocdataStatus
ocdata_add_plugin(
  ocdataContext* ctx,
  ocdataPlugin* plugin
);

ocdataStatus
ocdata_add_comp_option(
  ocdataContext* ctx,
  ocdataCompressionOption* option
);

ocdataStatus
ocdata_add_comp_options(
  ocdataContext* ctx,
  List* options
);

ocdataStatus
ocdata_add_codec(
  ocdataContext* ctx,
  ocdataCodec* codec
);

ocdataStatus
ocdata_add_comp(
  ocdataContext* ctx,
  ocdataCompresion* compression
);

ocdataStatus
ocdata_add_result(
  ocdataContext* ctx,
  ocdataResult* result
);

ocdataStatus
ocdata_get_file(
  ocdataContext* ctx,
  ocdataFile** res,
  int64_t file_id
);

ocdataStatus
ocdata_get_plugin(
  ocdataContext* ctx,
  ocdataPlugin** res,
  int64_t plugin_id
);

ocdataStatus
ocdata_get_comp_option(
  ocdataContext* ctx,
  ocdataCompressionOption** res,
  int64_t comp_id,
  int64_t option_id
);

ocdataStatus
ocdata_get_comp_options(
  ocdataContext* ctx,
  List** options,
  int64_t comp_id
);

ocdataStatus
ocdata_get_codec(
  ocdataContext* ctx,
  ocdataCodec** res,
  int64_t codec_id
);

ocdataStatus
ocdata_get_comp(
  ocdataContext* ctx,
  ocdataCompresion** res,
  int64_t comp_id
);

ocdataStatus
ocdata_get_result(
  ocdataContext* ctx,
  ocdataResult** res,
  int64_t file_id,
  int64_t comp_id
);

ocdataStatus
ocdata_destroy_context(
  ocdataContext **ctx
);


ocdataStatus
ocdata_garbage_collect();

#endif
