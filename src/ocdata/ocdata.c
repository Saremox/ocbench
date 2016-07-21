#include <stdio.h>
#include "ocdata.h"

ocdataStatus
ocdata_create_context(ocdataContext** ctx, char* dbfilepath, int64_t flags)
{

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
