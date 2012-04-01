# "global.mk"
gitdiffs-cached: force
	-{ cd $(srcdir) && git diff --cached . ;} > $@
gitdiffs-uncached: force
	-{ cd $(srcdir) && git diff . ;} > $@
gitdiffs: gitdiffs-cached gitdiffs-uncached

force:
.PHONY: force
