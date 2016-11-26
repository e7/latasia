AUX_SOURCE_DIRECTORY(. src_modules)
ADD_LIBRARY(lts_modules STATIC ${src_modules})
TARGET_LINK_LIBRARIES(lts_modules lua)
