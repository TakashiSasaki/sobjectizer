/*
 * An advanced test for extensible select.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;
using namespace chrono;
using namespace chrono_literals;

void
do_check_on_empty_cases_list( const so_5::mchain_t & chain )
{
	so_5::send<std::string>(chain, "Hello!");

	auto sel = make_extensible_select(
			so_5::from_all().handle_all().empty_timeout( 20ms ),
			case_( chain ) );

	auto r = select( sel );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 0 == r.handled() );
	UT_CHECK_CONDITION(
			so_5::mchain_props::extraction_status_t::msg_extracted == r.status() );
}

UT_UNIT_TEST( test_empty_cases_list )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_on_empty_cases_list(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_timeout_on_empty_queue: " + p.first );
	}
}

void
do_check_timeout_on_empty_queue( const so_5::mchain_t & chain )
{
	std::thread child{ [&] {
		auto sel = make_extensible_select(
				so_5::from_all().handle_all().empty_timeout( 500ms ),
				case_( chain ) );

		auto r = select( sel );

		UT_CHECK_CONDITION( 0 == r.extracted() );
		UT_CHECK_CONDITION(
				so_5::mchain_props::extraction_status_t::no_messages == r.status() );
	} };

	child.join();
}

UT_UNIT_TEST( test_timeout_on_empty_queue )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_timeout_on_empty_queue(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_timeout_on_empty_queue: " + p.first );
	}
}

void
do_check_total_time( const so_5::mchain_t & chain )
{
	so_5::send< int >( chain, 0 );
	so_5::send< int >( chain, 1 );
	so_5::send< string >( chain, "hello!" );

	std::thread child{ [&] {
		auto r = select(
				make_extensible_select(
						so_5::from_all().handle_all().total_time( 500ms ),
						case_( chain, []( const std::string & ) {} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION(
				so_5::mchain_props::extraction_status_t::msg_extracted == r.status() );
	} };

	child.join();
}

UT_UNIT_TEST( test_total_time )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_total_time(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_total_time: " + p.first );
	}
}

void
do_check_handle_n(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		auto r = select(
				make_extensible_select( 
						so_5::from_all().handle_n( 3 ),
						case_( ch1,
							[&ch2]( int i ) {
								so_5::send< int >( ch2, i );
							} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 3 == r.handled() );
	} };

	so_5::send< int >( ch1, 0 );
	auto r = select(
			make_extensible_select(
					so_5::from_all().handle_n( 2 ),
					case_( ch2,
						[&ch1]( int i ) {
							so_5::send< int >( ch1, i + 1 );
						} ) )
			);;

	UT_CHECK_CONDITION( 2 == r.extracted() );
	UT_CHECK_CONDITION( 2 == r.handled() );

	child.join();
}

UT_UNIT_TEST( test_handle_n )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_handle_n(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_handle_n: " + p.first );
	}
}

void
do_check_extract_n(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		auto r = select(
				make_extensible_select( 
						so_5::from_all().handle_n( 3 ).extract_n( 3 ),
						case_( ch1,
							[&ch2]( int i ) {
								so_5::send< int >( ch2, i );
							} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
	} };

	so_5::send< string >( ch1, "0" );
	so_5::send< int >( ch1, 0 );

	auto r = select(
			make_extensible_select(
					so_5::from_all().handle_n( 1 ),
					case_( ch2,
						[&ch1]( int i ) {
							so_5::send< string >( ch1, to_string( i + 1 ) );
							so_5::send< int >( ch1, i + 1 );
						} ) )
			);

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );

	child.join();
}

UT_UNIT_TEST( test_extract_n )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_extract_n(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_extract_n: " + p.first );
	}
}

void
do_check_stop_pred(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		int last_received = 0;
		auto r = select( make_extensible_select(
				so_5::from_all().handle_all().stop_on(
						[&last_received] { return last_received > 10; } ),
				case_( ch1,
					[&ch2, &last_received]( int i ) {
						last_received = i;
						so_5::send< int >( ch2, i );
					} ) )
		);

		UT_CHECK_CONDITION( r.extracted() > 10 );
		UT_CHECK_CONDITION( r.handled() > 10 );
	} };

	int i = 0;
	so_5::send< int >( ch1, i );
	auto r = select( make_extensible_select(
			so_5::from_all().handle_all().stop_on( [&i] { return i > 10; } ),
			case_( ch2,
				[&ch1, &i]( int ) {
					++i;
					so_5::send< int >( ch1, i );
				} ) )
	);

	UT_CHECK_CONDITION( r.extracted() > 10 );
	UT_CHECK_CONDITION( r.handled() > 10 );

	child.join();
}

UT_UNIT_TEST( test_stop_pred )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_stop_pred(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_stop_pred(no_msg_tracing): " + p.first );

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env(
						[]( so_5::environment_t & ) {},
						[]( so_5::environment_params_t & env_params ) {
							env_params.message_delivery_tracer(
									so_5::msg_tracing::std_clog_tracer() );
						} );

				do_check_stop_pred(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_stop_pred(msg_tracing): " + p.first );
	}
}

void
do_check_nested_select(
	const so_5::mchain_t & ch )
{
	struct try_nested_select {};

	std::optional<int> error;

	std::thread child{ [&] {
		auto sel = make_extensible_select(
				so_5::from_all().handle_all() );

		add_select_cases(
				sel,
				case_( ch,
					[&]( const try_nested_select & ) {
						try {
							select( sel ); // Nested is prohibited.
						}
						catch( const so_5::exception_t & x ) {
							error = x.error_code();
						}
					} ) );

		auto r = select( sel );

		UT_CHECK_CONDITION( 1 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION( error &&
				so_5::rc_extensible_select_is_active_now == *error );
	} };

	so_5::send< try_nested_select >( ch );
	so_5::close_retain_content( ch );

	child.join();
}

UT_UNIT_TEST( test_nested_select )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_nested_select(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_nested_select: " + p.first );
	}
}

void
do_check_modify_from_select(
	const so_5::mchain_t & ch )
{
	struct try_modify {};
	struct empty {};

	std::optional<int> error;

	std::thread child{ [&] {
		auto sel = make_extensible_select(
				so_5::from_all().handle_all() );

		add_select_cases(
				sel,
				case_( ch,
					[&]( const try_modify & ) {
						try {
							add_select_cases( sel,
									case_( ch, []( const empty & ) {} ) );
						}
						catch( const so_5::exception_t & x ) {
							error = x.error_code();
						}
					} ) );

		auto r = select( sel );

		UT_CHECK_CONDITION( 1 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION( error &&
				so_5::rc_extensible_select_is_active_now == *error );
	} };

	so_5::send< try_modify >( ch );
	so_5::close_retain_content( ch );

	child.join();
}

UT_UNIT_TEST( test_modify_from_select )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_modify_from_select(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_nested_select: " + p.first );
	}
}

int
main()
{
	UT_RUN_UNIT_TEST( test_empty_cases_list )
	UT_RUN_UNIT_TEST( test_timeout_on_empty_queue )
	UT_RUN_UNIT_TEST( test_total_time )
	UT_RUN_UNIT_TEST( test_handle_n )
	UT_RUN_UNIT_TEST( test_extract_n )
	UT_RUN_UNIT_TEST( test_stop_pred )
	UT_RUN_UNIT_TEST( test_nested_select )
	UT_RUN_UNIT_TEST( test_modify_from_select )

	return 0;
}

