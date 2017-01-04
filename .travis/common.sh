# This is not really a SH file!
#----------------------------------------
# RULES for this file
#----------------------------------------
# All variables must be in the ${NAME} form with curly braces.
#----------------------------------------
# This file can contain:  NAME=${VALUE_${NESTED_VALUE2}}
#----------------------------------------
# This file must not contain any 'script language'
# Other tools read this file... and assume simple "name=value' content.
#----------------------------------------

# This is where things are placed
CACHED_DIR=/tmp/openthread.cache

ASTYLE_url=http://jaist.dl.sourceforge.net/project/astyle/astyle/astyle%202.05.1/astyle_2.05.1_linux.tar.gz
ASTYLE_path=${CACHED_DIR}/astyle/build/gcc/bin
ASTYLE_file=astyle_2.05.1_linux.tar.gz


ARM_GCC_49_url=https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
ARM_GCC_49_path=${CACHED_DIR}/gcc-arm-none-eabi-4_9-2015q3/bin
ARM_GCC_49_file=gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

ARM_GCC_54_url=https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q3-update/+download/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2
ARM_GCC_54_path=${CACHED_DIR}/gcc-arm-none-eabi-5_4-2016q3/bin
ARM_GCC_54_file=gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2

ARM_GCC_49_mac_url=https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2
ARM_GCC_49_mac_file=gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2
ARM_GCC_49_mac_path=${CACHED_DIR}/gcc-arm-none-eabi-4_9-2015q3/bin
