// A second translation unit catches header-level multiple-definition errors.
#include <klang.h>
int ue_additions_version() { return klang::version.minor; }
