find_package(DRRAUtils REQUIRED)

# Shared RTL modules, each a Bender package.
#
# NOTE the output is deliberately one level flatter than the source: these are
# copied to library/common/<name>, not library/common/rtl/<name>. The rtl/
# grouping is a source-tree convenience only.
#
# The dependency paths in the resources (../../common/<name>) are a contract
# with vesyla's *work tree*, not with this repo: vesyla re-lays the components
# out as work/system/rtl/common/<package-name> and copies each Bender.yml
# verbatim, so a path with an extra rtl/ level resolves to nothing there and
# the dependency is silently dropped from the compile order.
add_drra_folder(common)
