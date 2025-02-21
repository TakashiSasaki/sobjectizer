/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Default implementation of multithreaded environment infrastructure.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/environment_infrastructure.hpp>
#include <so_5/mchain.hpp>

#include <so_5/disp/one_thread/pub.hpp>

#include <so_5/impl/coop_repository_basis.hpp>

#include <so_5/stats/impl/std_controller.hpp>

#include <so_5/timers.hpp>

namespace so_5 {

namespace env_infrastructures {

namespace default_mt {

namespace impl {

//
// coop_repo_t
//
/*!
 * \brief Implementation of coop_repository for
 * multithreaded environment infrastructure.
 *
 * \since
 * v.5.5.19
 */
class coop_repo_t final : protected ::so_5::impl::coop_repository_basis_t
	{
	public :
		//! Initializing constructor.
		coop_repo_t(
			//! SObjectizer Environment.
			outliving_reference_t< environment_t > env,
			//! Cooperation action listener.
			coop_listener_unique_ptr_t coop_listener );

		//! Do initialization.
		void
		start();

		//! Finish work.
		/*!
		 * Initiates deregistration of all agents. Waits for complete
		 * deregistration for all of them. Waits for termination of
		 * cooperation deregistration thread.
		 */
		void
		finish();

		using coop_repository_basis_t::make_coop;

		using coop_repository_basis_t::register_coop;

		//! Notification about readiness of the cooperation deregistration.
		void
		ready_to_deregister_notify(
			coop_shptr_t coop );

		//! Do final actions of the cooperation deregistration.
		/*!
		 * \retval true there are some live cooperations.
		 * \retval false there is no more live cooperations.
		 */
		bool
		final_deregister_coop(
			//! Cooperation to be deregistered.
			coop_shptr_t coop );

		//! Initiate start of the cooperation deregistration.
		void
		start_deregistration();

		//! Wait for a signal about start of the cooperation deregistration.
		void
		wait_for_start_deregistration();

		//! Wait for end of all cooperations deregistration.
		void
		wait_all_coop_to_deregister();

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Get the current statistic for run-time monitoring.
		 */
		environment_infrastructure_t::coop_repository_stats_t
		query_stats();

	private :
		//! Condition variable for the deregistration start indication.
		std::condition_variable m_deregistration_started_cond;

		//! Condition variable for the deregistration finish indication.
		std::condition_variable m_deregistration_finished_cond;

		/*!
		 * \name Stuff for final coop deregistration.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief Queue of coops to be finally deregistered.
		 *
		 * \note Actual mchain is created inside start() method.
		 */
		mchain_t m_final_dereg_chain;

		/*!
		 * \since
		 * v.5.5.13
		 *
		 * \brief A separate thread for doing the final deregistration.
		 *
		 * \note Actual thread is started inside start() method.
		 */
		std::thread m_final_dereg_thread;
		/*!
		 * \}
		 */
	};

//
// mt_env_infrastructure_t
//
/*!
 * \brief Default implementation of multithreaded environment infrastructure.
 *
 * \since
 * v.5.5.19
 */
class mt_env_infrastructure_t
	: public environment_infrastructure_t
	{
	public :
		mt_env_infrastructure_t(
			//! Environment to work in.
			environment_t & env,
			//! Parameters for the default dispatcher,
			so_5::disp::one_thread::disp_params_t default_disp_params,
			//! Timer thread to be used by environment.
			timer_thread_unique_ptr_t timer_thread,
			//! Cooperation action listener.
			coop_listener_unique_ptr_t coop_listener,
			//! Run-time stats distribution mbox.
			mbox_t stats_distribution_mbox );

		virtual void
		launch( env_init_t init_fn ) override;

		virtual void
		stop() override;

		SO_5_NODISCARD
		virtual coop_unique_holder_t
		make_coop(
			coop_handle_t parent,
			disp_binder_shptr_t default_binder ) override;

		virtual coop_handle_t
		register_coop(
			coop_unique_holder_t coop ) override;

		virtual void
		ready_to_deregister_notify(
			coop_shptr_t coop ) noexcept override;

		virtual bool
		final_deregister_coop(
			coop_shptr_t coop_name ) override;

		virtual so_5::timer_id_t
		schedule_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override;

		virtual void
		single_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause ) override;

		virtual ::so_5::stats::controller_t &
		stats_controller() noexcept override;

		virtual ::so_5::stats::repository_t &
		stats_repository() noexcept override;

		virtual coop_repository_stats_t
		query_coop_repository_stats() override;

		virtual timer_thread_stats_t
		query_timer_thread_stats() override;

		virtual disp_binder_shptr_t
		make_default_disp_binder() override;

	private :
		environment_t & m_env;

		//! Parameters for the default dispatcher.
		/*!
		 * \note
		 * There wasn't such attribute in previous versions of SObjectizer-5
		 * because creation and running of the default dispatcher was separated.
		 * In v.5.6.0 the default dispatcher is created inside lauch() and
		 * we have to store parameters for the default dispatcher somewhere.
		 *
		 * \since
		 * v.5.6.0
		 */
		const disp::one_thread::disp_params_t m_default_dispatcher_params;

		//! Default dispatcher.
		/*!
		 * \attention
		 * The actual value is created only in
		 * run_default_dispatcher_and_go_further() function. And this
		 * value is dropped after return from that function.
		 * It means that default dispatcher exists only while
		 * lauch() is running.
		 */
		disp::one_thread::dispatcher_handle_t m_default_dispatcher;

		//! Timer thread to be used by the environment.
		timer_thread_unique_ptr_t m_timer_thread;

		//! Repository of registered cooperations.
		coop_repo_t m_coop_repo;

		//! Run-time stats controller to be used by the environment.
		::so_5::stats::impl::std_controller_t m_stats_controller;

		void
		run_default_dispatcher_and_go_further( env_init_t init_fn );

		void
		run_timer_thread_and_go_further( env_init_t init_fn );

		void
		run_agent_core_and_go_further( env_init_t init_fn );

		void
		run_user_supplied_init_and_wait_for_stop( env_init_t init_fn );
	};

} /* namespace impl */

} /* namespace default_mt */

} /* namespace env_infrastructures */

} /* namespace so_5 */


