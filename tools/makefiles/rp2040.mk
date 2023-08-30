
# Add the runtime
EXTRA_ELF_DEPS += ${BUILD_ROOT}/runtime/libruntime.a
CROSS_FLAGS := -mcpu=cortex-m0plus -mtune=cortex-m0plus -march=armv6s-m -mfloat-abi=soft -mthumb -mno-unaligned-access --specs=${PROJECT_ROOT}/tools/picolibc-rp2040.specs
LDFLAGS += -L${BUILD_ROOT}/runtime

# Add wrappers to hardware divider support */
LDFLAGS += -Wl,--wrap=__aeabi_idiv
LDFLAGS += -Wl,--wrap=__aeabi_idivmod
LDFLAGS += -Wl,--wrap=__aeabi_ldivmod
LDFLAGS += -Wl,--wrap=__aeabi_uidiv
LDFLAGS += -Wl,--wrap=__aeabi_uidivmod
LDFLAGS += -Wl,--wrap=__aeabi_uldivmod

# Add wrapper to float rom code
LDFLAGS += -Wl,--wrap=__aeabi_fadd
LDFLAGS += -Wl,--wrap=__aeabi_fdiv
LDFLAGS += -Wl,--wrap=__aeabi_fmul
LDFLAGS += -Wl,--wrap=__aeabi_frsub
LDFLAGS += -Wl,--wrap=__aeabi_fsub
LDFLAGS += -Wl,--wrap=__aeabi_cfcmpeq
LDFLAGS += -Wl,--wrap=__aeabi_cfrcmple
LDFLAGS += -Wl,--wrap=__aeabi_cfcmple
LDFLAGS += -Wl,--wrap=__aeabi_fcmpeq
LDFLAGS += -Wl,--wrap=__aeabi_fcmplt
LDFLAGS += -Wl,--wrap=__aeabi_fcmple
LDFLAGS += -Wl,--wrap=__aeabi_fcmpge
LDFLAGS += -Wl,--wrap=__aeabi_fcmpgt
LDFLAGS += -Wl,--wrap=__aeabi_fcmpun
LDFLAGS += -Wl,--wrap=__aeabi_i2f
LDFLAGS += -Wl,--wrap=__aeabi_l2f
LDFLAGS += -Wl,--wrap=__aeabi_ui2f
LDFLAGS += -Wl,--wrap=__aeabi_ul2f
LDFLAGS += -Wl,--wrap=__aeabi_f2iz
LDFLAGS += -Wl,--wrap=__aeabi_f2lz
LDFLAGS += -Wl,--wrap=__aeabi_f2uiz
LDFLAGS += -Wl,--wrap=__aeabi_f2ulz
LDFLAGS += -Wl,--wrap=__aeabi_f2d
LDFLAGS += -Wl,--wrap=sqrtf
LDFLAGS += -Wl,--wrap=cosf
LDFLAGS += -Wl,--wrap=sinf
LDFLAGS += -Wl,--wrap=tanf
LDFLAGS += -Wl,--wrap=atan2f
LDFLAGS += -Wl,--wrap=expf
LDFLAGS += -Wl,--wrap=logf
LDFLAGS += -Wl,--wrap=ldexpf
LDFLAGS += -Wl,--wrap=copysignf
LDFLAGS += -Wl,--wrap=truncf
LDFLAGS += -Wl,--wrap=floorf
LDFLAGS += -Wl,--wrap=ceilf
LDFLAGS += -Wl,--wrap=roundf
LDFLAGS += -Wl,--wrap=sincosf
LDFLAGS += -Wl,--wrap=asinf
LDFLAGS += -Wl,--wrap=acosf
LDFLAGS += -Wl,--wrap=atanf
LDFLAGS += -Wl,--wrap=sinhf
LDFLAGS += -Wl,--wrap=coshf
LDFLAGS += -Wl,--wrap=tanhf
LDFLAGS += -Wl,--wrap=asinhf
LDFLAGS += -Wl,--wrap=acoshf
LDFLAGS += -Wl,--wrap=atanhf
LDFLAGS += -Wl,--wrap=exp2f
LDFLAGS += -Wl,--wrap=log2f
LDFLAGS += -Wl,--wrap=exp10f
LDFLAGS += -Wl,--wrap=log10f
LDFLAGS += -Wl,--wrap=powf
LDFLAGS += -Wl,--wrap=powintf
LDFLAGS += -Wl,--wrap=hypotf
LDFLAGS += -Wl,--wrap=cbrtf
LDFLAGS += -Wl,--wrap=fmodf
LDFLAGS += -Wl,--wrap=dremf
LDFLAGS += -Wl,--wrap=remainderf
LDFLAGS += -Wl,--wrap=remquof
LDFLAGS += -Wl,--wrap=expm1f
LDFLAGS += -Wl,--wrap=log1pf
LDFLAGS += -Wl,--wrap=fmaf

# Add wrapper to double rom code
LDFLAGS += -Wl,--wrap=__aeabi_dadd
LDFLAGS += -Wl,--wrap=__aeabi_ddiv
LDFLAGS += -Wl,--wrap=__aeabi_dmul
LDFLAGS += -Wl,--wrap=__aeabi_drsub
LDFLAGS += -Wl,--wrap=__aeabi_dsub
LDFLAGS += -Wl,--wrap=__aeabi_cdcmpeq
LDFLAGS += -Wl,--wrap=__aeabi_cdrcmple
LDFLAGS += -Wl,--wrap=__aeabi_cdcmple
LDFLAGS += -Wl,--wrap=__aeabi_dcmpeq
LDFLAGS += -Wl,--wrap=__aeabi_dcmplt
LDFLAGS += -Wl,--wrap=__aeabi_dcmple
LDFLAGS += -Wl,--wrap=__aeabi_dcmpge
LDFLAGS += -Wl,--wrap=__aeabi_dcmpgt
LDFLAGS += -Wl,--wrap=__aeabi_dcmpun
LDFLAGS += -Wl,--wrap=__aeabi_i2d
LDFLAGS += -Wl,--wrap=__aeabi_l2d
LDFLAGS += -Wl,--wrap=__aeabi_ui2d
LDFLAGS += -Wl,--wrap=__aeabi_ul2d
LDFLAGS += -Wl,--wrap=__aeabi_d2iz
LDFLAGS += -Wl,--wrap=__aeabi_d2lz
LDFLAGS += -Wl,--wrap=__aeabi_d2uiz
LDFLAGS += -Wl,--wrap=__aeabi_d2ulz
LDFLAGS += -Wl,--wrap=__aeabi_d2f
LDFLAGS += -Wl,--wrap=sqrt
LDFLAGS += -Wl,--wrap=cos
LDFLAGS += -Wl,--wrap=sin
LDFLAGS += -Wl,--wrap=tan
LDFLAGS += -Wl,--wrap=atan2
LDFLAGS += -Wl,--wrap=exp
LDFLAGS += -Wl,--wrap=log
LDFLAGS += -Wl,--wrap=ldexp
LDFLAGS += -Wl,--wrap=copysign
LDFLAGS += -Wl,--wrap=trunc
LDFLAGS += -Wl,--wrap=floor
LDFLAGS += -Wl,--wrap=ceil
LDFLAGS += -Wl,--wrap=round
LDFLAGS += -Wl,--wrap=sincos
LDFLAGS += -Wl,--wrap=asin
LDFLAGS += -Wl,--wrap=acos
LDFLAGS += -Wl,--wrap=atan
LDFLAGS += -Wl,--wrap=sinh
LDFLAGS += -Wl,--wrap=cosh
LDFLAGS += -Wl,--wrap=tanh
LDFLAGS += -Wl,--wrap=asinh
LDFLAGS += -Wl,--wrap=acosh
LDFLAGS += -Wl,--wrap=atanh
LDFLAGS += -Wl,--wrap=exp2
LDFLAGS += -Wl,--wrap=log2
LDFLAGS += -Wl,--wrap=exp10
LDFLAGS += -Wl,--wrap=log10
LDFLAGS += -Wl,--wrap=pow
LDFLAGS += -Wl,--wrap=powint
LDFLAGS += -Wl,--wrap=hypot
LDFLAGS += -Wl,--wrap=cbrt
LDFLAGS += -Wl,--wrap=fmod
LDFLAGS += -Wl,--wrap=drem
LDFLAGS += -Wl,--wrap=remainder
LDFLAGS += -Wl,--wrap=remquo
LDFLAGS += -Wl,--wrap=expm1
LDFLAGS += -Wl,--wrap=log1p
LDFLAGS += -Wl,--wrap=fma

# Add wraper to bit ops
LDFLAGS += -Wl,--wrap=__clzsi2
LDFLAGS += -Wl,--wrap=__clzdi2
LDFLAGS += -Wl,--wrap=__ctzsi2
LDFLAGS += -Wl,--wrap=__ctzdi2
LDFLAGS += -Wl,--wrap=__popcountsi2
LDFLAGS += -Wl,--wrap=__popcountdi2
LDFLAGS += -Wl,--wrap=__clz
LDFLAGS += -Wl,--wrap=__clzl
LDFLAGS += -Wl,--wrap=__clzsi2
LDFLAGS += -Wl,--wrap=__clzll

# Add wrapper to int64 ops
LDFLAGS += -Wl,--wrap=__aeabi_lmul

# Add wrapper to mem ops
LDFLAGS += -Wl,--wrap=memcpy
LDFLAGS += -Wl,--wrap=memset
LDFLAGS += -Wl,--wrap=__aeabi_memcpy
LDFLAGS += -Wl,--wrap=__aeabi_memset
LDFLAGS += -Wl,--wrap=__aeabi_memcpy4
LDFLAGS += -Wl,--wrap=__aeabi_memset4
LDFLAGS += -Wl,--wrap=__aeabi_memcpy8
LDFLAGS += -Wl,--wrap=__aeabi_memset8

# Add tls wrapper to multicore tls
LDFLAGS += -Wl,--wrap=__aeabi_read_tp
LDFLAGS += -Wl,--wrap=_set_tls
LDFLAGS += -Wl,--wrap=_init_tls
