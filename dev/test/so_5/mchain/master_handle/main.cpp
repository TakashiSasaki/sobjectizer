/*
 * Test for mchain_master_handle_t class.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include "../mchain_params.hpp"

using namespace std;

void handle_unexpected( int i )
{
	throw runtime_error( "unexpected message: " + to_string(i) );
}

void handle_expected( int i )
{
	if( i != 42 )
		throw runtime_error( "unexpected value: " + to_string(i) );
}

void
check_drop_content1(
	const so_5::mchain_t & ch1 )
{
	{
		auto master = so_5::mchain_master_handle_t::with_drop_content( ch1 );

		so_5::send< int >( *master, 0 );
	}

	auto r = so_5::select(
			so_5::from_all().handle_n(1).no_wait_on_empty(),
			case_( ch1, []( int i ) { handle_unexpected(i); } ) );

	if( r.status() != so_5::mchain_props::extraction_status_t::chain_closed )
		throw runtime_error( "unexpected value of so_5::select "
				"return code: " + to_string( static_cast<int>(r.status()) ) );
}

void
check_drop_content3(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2,
	const so_5::mchain_t & ch3 )
{
	{
		auto master1 = so_5::mchain_master_handle_t::with_drop_content( ch1 );
		auto master2 = so_5::mchain_master_handle_t::with_drop_content( ch2 );
		auto master3 = so_5::mchain_master_handle_t::with_drop_content( ch3 );

		so_5::send< int >( *master1, 0 );
		so_5::send< int >( *master2, 1 );
		so_5::send< int >( *master3, 2 );
	}

	auto r = so_5::select(
			so_5::from_all().handle_n(1).no_wait_on_empty(),
			case_( ch1, []( int i ) { handle_unexpected(i); } ),
			case_( ch2, []( int i ) { handle_unexpected(i); } ),
			case_( ch3, []( int i ) { handle_unexpected(i); } ) );

	if( r.status() != so_5::mchain_props::extraction_status_t::chain_closed )
		throw runtime_error( "unexpected value of so_5::select "
				"return code: " + to_string( static_cast<int>(r.status()) ) );
}

void
check_retain_content1(
	const so_5::mchain_t & ch1 )
{
	{
		auto master = so_5::mchain_master_handle_t::with_retain_content( ch1 );

		so_5::send< int >( *master, 42 );
		so_5::send< int >( *master, 42 );
	}

	auto r = so_5::select(
			so_5::from_all().handle_n(2),
			case_( ch1, []( int i ) { handle_expected(i); } ) );

	if( 2 != r.handled() )
		throw runtime_error( "unexpected count of handled messages: " +
				to_string( r.handled() ) );

	r = so_5::select(
			so_5::from_all().handle_n(1).no_wait_on_empty(),
			case_( ch1, []( int i ) { handle_unexpected(i); } ) );

	if( r.status() != so_5::mchain_props::extraction_status_t::chain_closed )
		throw runtime_error( "unexpected value of so_5::select "
				"return code: " + to_string( static_cast<int>(r.status()) ) );
}

void
check_retain_content3(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2,
	const so_5::mchain_t & ch3 )
{
	{
		auto master1 = so_5::mchain_master_handle_t::with_retain_content( ch1 );
		auto master2 = so_5::mchain_master_handle_t::with_retain_content( ch2 );
		auto master3 = so_5::mchain_master_handle_t::with_retain_content( ch3 );

		so_5::send< int >( *master1, 42 );
		so_5::send< int >( *master1, 42 );

		so_5::send< int >( *master2, 42 );
		so_5::send< int >( *master2, 42 );

		so_5::send< int >( *master3, 42 );
		so_5::send< int >( *master3, 42 );
	}

	auto r = so_5::select(
			so_5::from_all().handle_n(6),
			case_( ch1, []( int i ) { handle_expected(i); } ),
			case_( ch2, []( int i ) { handle_expected(i); } ),
			case_( ch3, []( int i ) { handle_expected(i); } ) );

	if( 6 != r.handled() )
		throw runtime_error( "unexpected count of handled messages: " +
				to_string( r.handled() ) );

	r = so_5::select(
			so_5::from_all().handle_n(1).no_wait_on_empty(),
			case_( ch1, []( int i ) { handle_unexpected(i); } ),
			case_( ch2, []( int i ) { handle_unexpected(i); } ),
			case_( ch3, []( int i ) { handle_unexpected(i); } ) );

	if( r.status() != so_5::mchain_props::extraction_status_t::chain_closed )
		throw runtime_error( "unexpected value of so_5::select "
				"return code: " + to_string( static_cast<int>(r.status()) ) );
}

void
do_check( bool msg_tracing_enabled )
{
	run_with_time_limit(
		[msg_tracing_enabled]()
		{
			so_5::wrapped_env_t env{
				[]( so_5::environment_t & ) {},
				[msg_tracing_enabled]( so_5::environment_params_t & params ) {
					if( msg_tracing_enabled )
						params.message_delivery_tracer(
								so_5::msg_tracing::std_clog_tracer() );
				} };

			auto params = build_mchain_params();

			for( const auto & p : params )
			{
				cout << "=== " << p.first << " ===" << endl;

				check_drop_content1(
						env.environment().create_mchain( p.second ) );

				check_drop_content3(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );

				check_retain_content1(
						env.environment().create_mchain( p.second ) );

				check_retain_content3(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			}
		},
		20 );
}

int
main()
{
	try
	{
		do_check( false );
		do_check( true );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}

