#ifndef OCALGORITHM_INC
#define OCALGORITHM_INC
  typedef enum _error_code {
    INVALID_OPTION_KEY    = 1>>1,
    INVALID_OPTION_VALUE  = 1>>2
  } error_code;

  typedef enum _option_type{
    ALGORITHM_OPTION_INTEGER,
    ALGORITHM_OPTION_STRING
  } option_type;

  typedef struct _algorithm_option {
    char * name;
    option_type type;
    char * default_value;
  } algorithm_option;

  typedef struct _algorithm_option_value {
    char * key;
    char * value;
  } algorithm_option_value;

  const char * getName();
  algorithm_option * getAvailOptions();
  error_code setOptions(algorithm_option_value * options);
  error_code compress();
  error_code decompres();
  error_code initialize();
  error_code clean_up();

#endif
