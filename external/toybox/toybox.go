package toybox
import (
	"android/soong/android"
	"android/soong/cc"
	"strings"
)

func globalFlags(ctx android.LoadHookContext) []string {
	var cflags []string

// [ @CHC support carlife
    if spfIsTrue(ctx, "SEC_PRODUCT_FEATURE_COMMON_SUPPORT_CARLIFE") {
        cflags = append(cflags, "-DSEC_CARLIFE")
    }
// ] @CHC support carlife

	return cflags
}

func globalDefaults(ctx android.LoadHookContext) {
	type props struct {
		Cflags   []string
	}

	p := &props{}
	p.Cflags = globalFlags(ctx)

	ctx.AppendProperties(p)
}

func init() {
	android.RegisterModuleType("toybox_cc_defaults", toyboxDefaults)
}

func toyboxDefaults() android.Module {
	module := cc.DefaultsFactory()
	android.AddLoadHook(module, globalDefaults)

	return module
}

func spfIsTrue(ctx android.BaseContext, key string) bool {
    sec_product_feature_name, _ := ctx.AConfig().Getspf(key)
    return strings.EqualFold(sec_product_feature_name, "TRUE")
}
