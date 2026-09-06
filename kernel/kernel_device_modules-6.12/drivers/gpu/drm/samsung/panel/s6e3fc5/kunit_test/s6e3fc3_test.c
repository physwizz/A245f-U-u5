// SPDX-License-Identifier: GPL-2.0
/*
 * TODO: Add test description.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include "panel_debug.h"
#include "test_helper.h"
#include "s6e3fc3/s6e3fc3.h"
#include "oled_common.h"
#include "oled_common_dump.h"

/*
 * This is the most fundamental element of KUnit, the test case. A test case
 * makes a set EXPECTATIONs and ASSERTIONs about the behavior of some code; if
 * any expectations or assertions are not met, the test fails; otherwise, the
 * test passes.
 *
 * In KUnit, a test case is just a function with the signature
 * `void (*)(struct kunit *)`. `struct kunit` is a context object that stores
 * information about the current test.
 */

extern int s6e3fc3_maptbl_getidx_normal_hbm_transition(struct maptbl *tbl);

#if !defined(CONFIG_UML)
/* NOTE: Target running TC must be in the #ifndef CONFIG_UML */
static void s6e3fc3_test_foo(struct kunit *test)
{
	/*
	 * This is an EXPECTATION; it is how KUnit tests things. When you want
	 * to test a piece of code, you set some expectations about what the
	 * code should do. KUnit then runs the test and verifies that the code's
	 * behavior matched what was expected.
	 */
	KUNIT_EXPECT_EQ(test, 1, 2); // Obvious failure.
}
#endif

DECLARE_REDIRECT_MOCKABLE(s6e3fc3_calc_normal_hbm_transition_type, RETURNS(unsigned int), PARAMS(bool, bool));
DEFINE_FUNCTION_MOCK(s6e3fc3_calc_normal_hbm_transition_type,
		RETURNS(unsigned int), PARAMS(bool, bool));
DECLARE_REDIRECT_MOCKABLE(s6e3fc3_get_normal_hbm_transition_type, RETURNS(int), PARAMS(struct panel_bl_device *));
DEFINE_FUNCTION_MOCK(s6e3fc3_get_normal_hbm_transition_type,
		RETURNS(int), PARAMS(struct panel_bl_device *));

DEFINE_FUNCTION_MOCK(is_hbm_brightness,
		RETURNS(bool), PARAMS(struct panel_bl_device *, int));

static int s6e3fc3_normal_hbm_transition_property_update(struct panel_property *prop)
{
	struct panel_device *panel = prop->panel;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	return panel_property_set_enum_value(prop,
			s6e3fc3_get_normal_hbm_transition_type(panel_bl));
}

static void s6e3fc3_test_s6e3fc3_calc_normal_hbm_transition_type_function_success(struct kunit *test)
{
	bool NORMAL = false, HBM = true;

	KUNIT_EXPECT_EQ(test, (unsigned int)S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL,
		s6e3fc3_calc_normal_hbm_transition_type(NORMAL, NORMAL));
	KUNIT_EXPECT_EQ(test, (unsigned int)S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM,
		s6e3fc3_calc_normal_hbm_transition_type(NORMAL, HBM));
	KUNIT_EXPECT_EQ(test, (unsigned int)S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL,
		s6e3fc3_calc_normal_hbm_transition_type(HBM, NORMAL));
	KUNIT_EXPECT_EQ(test, (unsigned int)S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM,
		s6e3fc3_calc_normal_hbm_transition_type(HBM, HBM));
}

static void s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_type_function_fail_with_invalid_args(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, -EINVAL, s6e3fc3_get_normal_hbm_transition_type(NULL));
}

static void s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_type_function_fail_with_invalid_range(struct kunit *test)
{
	struct panel_device *panel = test->priv;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct mock_expectation *expectation_calc;
	struct mock_expectation *expectation_get;
	struct mock_expectation *expectation_hbm;

	expectation_hbm = Times(2, KUNIT_EXPECT_CALL(
			is_hbm_brightness(ptr_eq(test, panel_bl), any(test))));
	ActionOnMatch(expectation_hbm, bool_return(test, true));

	expectation_calc = Times(1, KUNIT_EXPECT_CALL(s6e3fc3_calc_normal_hbm_transition_type(any(test), any(test))));
	ActionOnMatch(expectation_calc, uint_return(test, (unsigned int)MAX_S6E3FC3_NORMAL_HBM_TRANSITION));

	expectation_get = Times(1, KUNIT_EXPECT_CALL(s6e3fc3_get_normal_hbm_transition_type(ptr_eq(test, panel_bl))));
	ActionOnMatch(expectation_get, INVOKE_REAL(test, s6e3fc3_get_normal_hbm_transition_type));

	KUNIT_EXPECT_EQ(test, -ERANGE, s6e3fc3_get_normal_hbm_transition_type(panel_bl));
}

static void s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_function_success(struct kunit *test)
{
	struct mock_expectation *expectation_normal, *expectation_hbm;
	struct panel_device *panel = test->priv;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	expectation_normal = Times(4, KUNIT_EXPECT_CALL(
			is_hbm_brightness(
				ptr_eq(test, panel_bl),
				int_le(test, 255))));
	expectation_hbm = Times(4, KUNIT_EXPECT_CALL(
			is_hbm_brightness(
				ptr_eq(test, panel_bl),
				int_ge(test, 256))));

	ActionOnMatch(expectation_normal, bool_return(test, false));
	ActionOnMatch(expectation_hbm, bool_return(test, true));

	panel_bl->props.prev_brightness = 22;
	panel_bl->props.brightness = 254;
	KUNIT_EXPECT_EQ(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL,
		s6e3fc3_get_normal_hbm_transition_type(panel_bl));

	panel_bl->props.prev_brightness = 254;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM,
		s6e3fc3_get_normal_hbm_transition_type(panel_bl));

	panel_bl->props.prev_brightness = 256;
	panel_bl->props.brightness = 255;
	KUNIT_EXPECT_EQ(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL,
		s6e3fc3_get_normal_hbm_transition_type(panel_bl));

	panel_bl->props.prev_brightness = 257;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM,
		s6e3fc3_get_normal_hbm_transition_type(panel_bl));
}

static void s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_fail_with_invalid_args(struct kunit *test)
{
	struct maptbl uninitialized_maptbl = { .pdata = NULL, };

	KUNIT_EXPECT_EQ(test, -EINVAL, s6e3fc3_maptbl_getidx_normal_hbm_transition(NULL));
	KUNIT_EXPECT_EQ(test, -EINVAL, s6e3fc3_maptbl_getidx_normal_hbm_transition(&uninitialized_maptbl));
}

static void s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_failsafe_select_index_0(struct kunit *test)
{
	struct mock_expectation *expectation1;
	struct panel_device *panel = test->priv;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	u8 r11s_dimming_speed_table[SMOOTH_TRANS_MAX][MAX_S6E3FC3_NORMAL_HBM_TRANSITION][1] = {
		[SMOOTH_TRANS_OFF] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x20 },
		},
		[SMOOTH_TRANS_ON] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x60 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x60 },
		},
	};
	struct pnobj_func f_init = __PNOBJ_FUNC_INITIALIZER(oled_maptbl_init_default, oled_maptbl_init_default);
	struct pnobj_func f_getidx = __PNOBJ_FUNC_INITIALIZER(s6e3fc3_maptbl_getidx_normal_hbm_transition, s6e3fc3_maptbl_getidx_normal_hbm_transition);
	struct pnobj_func f_copy = __PNOBJ_FUNC_INITIALIZER(oled_maptbl_copy_default, oled_maptbl_copy_default);
	struct maptbl trans_maptbl = DEFINE_3D_MAPTBL(r11s_dimming_speed_table, &f_init, &f_getidx, &f_copy);
	int FAILSAFE_INDEX_0 = 0;

	trans_maptbl.pdata = panel;
	trans_maptbl.initialized = true;

	expectation1 = Times(2, KUNIT_EXPECT_CALL(
			s6e3fc3_get_normal_hbm_transition_type(ptr_eq(test, panel_bl))));

	ActionOnMatch(expectation1, int_return(test, -EINVAL));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_OFF;
	panel_bl->props.prev_brightness = 257;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_OFF, FAILSAFE_INDEX_0, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));

	ActionOnMatch(expectation1, int_return(test, -ERANGE));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_ON;
	panel_bl->props.prev_brightness = 257;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_ON, FAILSAFE_INDEX_0, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));
}

static void s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_success(struct kunit *test)
{
	struct mock_expectation *expectation1;
	struct panel_device *panel = test->priv;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	u8 r11s_dimming_speed_table[SMOOTH_TRANS_MAX][MAX_S6E3FC3_NORMAL_HBM_TRANSITION][1] = {
		[SMOOTH_TRANS_OFF] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x20 },
		},
		[SMOOTH_TRANS_ON] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x60 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x60 },
		},
	};
	struct pnobj_func f_init = __PNOBJ_FUNC_INITIALIZER(oled_maptbl_init_default, oled_maptbl_init_default);
	struct pnobj_func f_getidx = __PNOBJ_FUNC_INITIALIZER(s6e3fc3_maptbl_getidx_normal_hbm_transition, s6e3fc3_maptbl_getidx_normal_hbm_transition);
	struct pnobj_func f_copy = __PNOBJ_FUNC_INITIALIZER(oled_maptbl_copy_default, oled_maptbl_copy_default);
	struct maptbl trans_maptbl = DEFINE_3D_MAPTBL(r11s_dimming_speed_table, &f_init, &f_getidx, &f_copy);

	trans_maptbl.pdata = panel;
	trans_maptbl.initialized = true;

	expectation1 = Times(4, KUNIT_EXPECT_CALL(
			s6e3fc3_get_normal_hbm_transition_type(ptr_eq(test, panel_bl))));

	ActionOnMatch(expectation1, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_OFF;
	panel_bl->props.prev_brightness = 22;
	panel_bl->props.brightness = 254;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_OFF,
		S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));

	ActionOnMatch(expectation1, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_ON;
	panel_bl->props.prev_brightness = 254;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_ON,
		S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));

	ActionOnMatch(expectation1, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_OFF;
	panel_bl->props.prev_brightness = 256;
	panel_bl->props.brightness = 255;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_OFF,
		S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));

	ActionOnMatch(expectation1, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM));
	panel_bl->props.smooth_transition = SMOOTH_TRANS_ON;
	panel_bl->props.prev_brightness = 257;
	panel_bl->props.brightness = 256;
	KUNIT_EXPECT_EQ(test, maptbl_index(&trans_maptbl, SMOOTH_TRANS_ON,
		S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM, 0),
		s6e3fc3_maptbl_getidx_normal_hbm_transition(&trans_maptbl));
}

static void s6e3fc3_test_s6e3fc3_hbm_transition_function_success(struct kunit *test)
{
	struct mock_expectation *expectation;
	struct panel_device *panel = test->priv;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	u8 r11s_hbm_transition_table[SMOOTH_TRANS_MAX][MAX_S6E3FC3_NORMAL_HBM_TRANSITION][1] = {
		[SMOOTH_TRANS_OFF] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x20 },
		},
		[SMOOTH_TRANS_ON] = {
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x28 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
			[S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x28 },
		},
	};
	struct maptbl hbm_onoff_maptbl = __OLED_MAPTBL_DEFAULT_INITIALIZER(r11s_hbm_transition_table,
			PANEL_BL_PROPERTY_SMOOTH_TRANSITION, S6E3FC3_NORMAL_HBM_TRANSITION_PROPERTY);

	hbm_onoff_maptbl.pdata = panel;
	hbm_onoff_maptbl.initialized = true;

	expectation = Times(4, KUNIT_EXPECT_CALL(
			s6e3fc3_get_normal_hbm_transition_type(ptr_eq(test, panel_bl))));

	ActionOnMatch(expectation, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL));
	panel_bl_set_property(panel_bl,
			&panel_bl->props.smooth_transition, SMOOTH_TRANS_OFF);
	KUNIT_EXPECT_EQ(test, maptbl_getidx(&hbm_onoff_maptbl),
			maptbl_index(&hbm_onoff_maptbl, SMOOTH_TRANS_OFF,
				S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL, 0));

	ActionOnMatch(expectation, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM));
	panel_bl_set_property(panel_bl,
			&panel_bl->props.smooth_transition, SMOOTH_TRANS_ON);
	KUNIT_EXPECT_EQ(test, maptbl_getidx(&hbm_onoff_maptbl),
			maptbl_index(&hbm_onoff_maptbl, SMOOTH_TRANS_ON,
				S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM, 0));

	ActionOnMatch(expectation, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL));
	panel_bl_set_property(panel_bl,
			&panel_bl->props.smooth_transition, SMOOTH_TRANS_OFF);
	KUNIT_EXPECT_EQ(test, maptbl_getidx(&hbm_onoff_maptbl),
			maptbl_index(&hbm_onoff_maptbl, SMOOTH_TRANS_OFF,
				S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL, 0));

	ActionOnMatch(expectation, int_return(test, S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM));
	panel_bl_set_property(panel_bl,
			&panel_bl->props.smooth_transition, SMOOTH_TRANS_ON);
	KUNIT_EXPECT_EQ(test, maptbl_getidx(&hbm_onoff_maptbl),
			maptbl_index(&hbm_onoff_maptbl, SMOOTH_TRANS_ON,
				S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM, 0));

}

/* NOTE: UML TC */
static void s6e3fc3_test_test(struct kunit *test)
{
	KUNIT_SUCCEED(test);
}

/*
 * This is run once before each test case, see the comment on
 * example_test_module for more information.
 */
static int s6e3fc3_test_init(struct kunit *test)
{
	struct panel_device *panel;
	extern struct list_head s6e3fc3_prop_list;
	struct panel_prop_enum_item s6e3fc3_normal_hbm_transition_enum_items[] = {
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL),
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM),
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL),
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(S6E3FC3_NORMAL_HBM_TRANSITION_HBM_TO_HBM),
	};
	struct panel_prop_enum_item smooth_transition_enum_items[] = {
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(SMOOTH_TRANS_OFF),
		__PANEL_PROPERTY_ENUM_ITEM_INITIALIZER(SMOOTH_TRANS_ON),
	};
	struct panel_prop_list s6e3fc3_property_array[] = {
		/* enum property */
		__DIMEN_PROPERTY_ENUM_INITIALIZER(S6E3FC3_NORMAL_HBM_TRANSITION_PROPERTY,
				S6E3FC3_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL, s6e3fc3_normal_hbm_transition_enum_items,
				s6e3fc3_normal_hbm_transition_property_update),
		/* range property */
		__PANEL_PROPERTY_ENUM_INITIALIZER(PANEL_BL_PROPERTY_SMOOTH_TRANSITION,
				SMOOTH_TRANS_ON, smooth_transition_enum_items),
	};

	panel = kunit_kzalloc(test, sizeof(*panel), GFP_KERNEL);

	test->priv = panel;

	INIT_LIST_HEAD(&panel->prop_list);
	KUNIT_ASSERT_EQ(test, panel_add_property_from_array(panel,
			s6e3fc3_property_array, ARRAY_SIZE(s6e3fc3_property_array)), 0);

	MOCK_SET_DEFAULT_ACTION_INVOKE_REAL(s6e3fc3_calc_normal_hbm_transition_type);
	MOCK_SET_DEFAULT_ACTION_INVOKE_REAL(s6e3fc3_get_normal_hbm_transition_type);

	return 0;
}

/*
 * This is run once after each test case, see the comment on example_test_module
 * for more information.
 */
static void s6e3fc3_test_exit(struct kunit *test)
{
}

/*
 * Here we make a list of all the test cases we want to add to the test module
 * below.
 */
static struct kunit_case s6e3fc3_test_cases[] = {
	/*
	 * This is a helper to create a test case object from a test case
	 * function; its exact function is not important to understand how to
	 * use KUnit, just know that this is how you associate test cases with a
	 * test module.
	 */
#if !defined(CONFIG_UML)
	/* NOTE: Target running TC */
	KUNIT_CASE(s6e3fc3_test_foo),
#endif
	/* NOTE: UML TC */
	KUNIT_CASE(s6e3fc3_test_test),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_calc_normal_hbm_transition_type_function_success),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_type_function_fail_with_invalid_args),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_type_function_fail_with_invalid_range),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_get_normal_hbm_transition_function_success),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_fail_with_invalid_args),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_failsafe_select_index_0),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_maptbl_getidx_normal_hbm_transition_function_success),
	KUNIT_CASE(s6e3fc3_test_s6e3fc3_hbm_transition_function_success),
	{},
};

/*
 * This defines a suite or grouping of tests.
 *
 * Test cases are defined as belonging to the suite by adding them to
 * `test_cases`.
 *
 * Often it is desirable to run some function which will set up things which
 * will be used by every test; this is accomplished with an `init` function
 * which runs before each test case is invoked. Similarly, an `exit` function
 * may be specified which runs after every test case and can be used to for
 * cleanup. For clarity, running tests in a test module would behave as follows:
 *
 * module.init(test);
 * module.test_case[0](test);
 * module.exit(test);
 * module.init(test);
 * module.test_case[1](test);
 * module.exit(test);
 * ...;
 */
static struct kunit_suite s6e3fc3_test_module = {
	.name = "s6e3fc3_test",
	.init = s6e3fc3_test_init,
	.exit = s6e3fc3_test_exit,
	.test_cases = s6e3fc3_test_cases,
};

/*
 * This registers the above test module telling KUnit that this is a suite of
 * tests that need to be run.
 */
kunit_test_suites(&s6e3fc3_test_module);

MODULE_LICENSE("GPL v2");
