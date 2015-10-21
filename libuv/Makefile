#
# Copyright (C) 2014 D-Team,ShenZhen
# lintel<lintel.huang@gmail.com>
#

include $(TOPDIR)/rules.mk

PKG_SOURCE_PROTO:=git
PKG_NAME:=libuv
PKG_VERSION:=1.7.5
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://github.com/libuv/libuv.git
PKG_SOURCE_VERSION:=master
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libuv
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=libuv shared library
  URL:=https://github.com/joyent/libuv
  DEPENDS:=+libc +uclibc++ +libpthread +librt  +libstdcpp
endef

define Package/libuv/description
 libuv is a multi-platform support library with a focus on asynchronous I/O.
endef

define Build/Prepare
	$(Build/Prepare/Default)
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
	./gyp_uv.py -Dlibrary=shared_library -Dtarget_arch=mips\
	);
endef

define Build/Compile
	cd $(PKG_BUILD_DIR);\
	make -C out \
	CC="$(TARGET_CC)" \
	CXX="$(TARGET_CROSS)g++" \
	AR="$(TARGET_CROSS)ar" \
	RANLIB="$(TARGET_CROSS)ranlib" \
	LINK="$(TARGET_CROSS)g++" \
	LD="$(TARGET_CROSS)ld" \
	LDFLAGS="-L$(TOOLCHAIN_DIR)/lib/ -Wl,-rpath-link $(TOOLCHAIN_DIR)/lib/" \
	CXXFLAGS="$(TARGET_CXXFLAGS) -nostdinc++"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/include/uv-private/
	$(CP) $(PKG_BUILD_DIR)/include/*.h $(1)/usr/include/
	$(CP) -rf $(PKG_BUILD_DIR)/include/uv-private/*.h $(1)/usr/include/uv-private/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/out/Debug/obj.target/libuv.so $(1)/usr/lib/
	ln -nsf libuv.so $(1)/usr/lib/libuv.so.1.7.5
endef

define Package/libuv/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/out/Debug/obj.target/libuv.so $(1)/usr/lib/
	ln -nsf libuv.so $(1)/usr/lib/libuv.so.1.7.5
endef

$(eval $(call BuildPackage,libuv))
