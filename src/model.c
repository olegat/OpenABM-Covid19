/*
 * model.c
 *
 *  Created on: 5 Mar 2020
 *      Author: hinchr
 */

#include "model.h"
#include "individual.h"
#include "utilities.h"
#include "constant.h"
#include "params.h"
#include <stdio.h>
#include <stdlib.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>

/*****************************************************************************************
*  Name:		new_model
*  Description: Builds a new model object from a parameters object and returns a
*  				pointer to it.
*  				 1. Creates memory for it
*  				 2. Initialises the gsl random numbers generator
*  Returns:		pointer to model
******************************************************************************************/
model* new_model( parameters *params )
{
	model *model  = calloc( 1, sizeof( model ) );
	model->params = *params;
	model->time   = 0;

	set_up_event_list( &(model->presymptomatic), params );
	set_up_event_list( &(model->asymptomatic), params );
	set_up_event_list( &(model->symptomatic), params );
	set_up_event_list( &(model->hospitalised), params );
	set_up_event_list( &(model->recovered), params );
	set_up_event_list( &(model->death), params );
	set_up_event_list( &(model->quarantined), params );
	set_up_event_list( &(model->quarantine_release), params );
	set_up_event_list( &(model->test_take), params );
	set_up_event_list( &(model->test_result), params );

	set_up_population( model );
	set_up_interactions( model );
	set_up_events( model );
	set_up_distributions( model );
	set_up_seed_infection( model );

	model->n_quarantine_days = 0;

	return model;
};

/*****************************************************************************************
*  Name:		destroy_model
*  Description: Destroys the model structure and releases its memory
******************************************************************************************/
void destroy_model( model *model )
{
	parameters *params = &(model->params);
	long idx;

	for( idx = 0; idx < params->n_total; idx++ )
		destroy_individual( &(model->population[idx] ) );

	free( model->population );
    free( model->possible_interactions );
    free( model->interactions );
    free( model );
};

/*****************************************************************************************
*  Name:		set_up_events
*  Description: sets up the event tags
*  Returns:		void
******************************************************************************************/
void set_up_events( model *model )
{
	parameters *params = &(model->params);
	long idx;
	int types = 6;

	model->events     = calloc( types * params->n_total, sizeof( event ) );
	model->next_event = &(model->events[0]);
	for( idx = 1; idx < types * params->n_total; idx++ )
	{
		model->events[idx-1].next = &(model->events[idx]);
		model->events[idx].last   = &(model->events[idx-1]);
	}
	model->events[types * params->n_total - 1].next = model->next_event;
	model->next_event->last = &(model->events[types * params->n_total - 1] );
}

/*****************************************************************************************
*  Name:		set_up_population
*  Description: sets up the initial population
*  Returns:		void
******************************************************************************************/
void set_up_population( model *model )
{
	parameters *params = &(model->params);
	long idx;

	model->population = calloc( params->n_total, sizeof( individual ) );
	for( idx = 0; idx < params->n_total; idx++ )
		initialize_individual( &(model->population[idx]), params, idx );
}

/*****************************************************************************************
*  Name:		set_up_interactions
*  Description: sets up the stock of interactions, note that these get recycled once we
*  				move to a later date
*  Returns:		void
******************************************************************************************/
void set_up_interactions( model *model )
{
	parameters *params = &(model->params);
	individual *indiv;
	long idx, n_idx, indiv_idx;

	// FIXME - need to a good estimate of the total number of interactions
	//         easy at the moment since we have a fixed number per individual
	long n_daily_interactions = params->n_total * params->mean_daily_interactions;
	long n_interactions       = n_daily_interactions * params->days_of_interactions;

	model->interactions          = calloc( n_interactions, sizeof( interaction ) );
	model->n_interactions        = n_interactions;
	model->interaction_idx       = 0;
	model->interaction_day_idx   = 0;

	model->possible_interactions = calloc( n_daily_interactions, sizeof( long ) );
	idx = 0;
	for( indiv_idx = 0; indiv_idx < params->n_total; indiv_idx++ )
	{
		indiv = &(model->population[ indiv_idx ]);
		for( n_idx = 0; n_idx < indiv->mean_interactions; n_idx++ )
			model->possible_interactions[ idx++ ] = indiv_idx;
	}

	model->n_possible_interactions = idx;
	model->n_total_intereactions   = 0;
}

/*****************************************************************************************
*  Name:		set_up_distributions
*  Description: sets up discrete distributions and functions which are used to
*  				model events
*  Returns:		void
******************************************************************************************/
void set_up_distributions( model *model )
{
	parameters *params = &(model->params);
	double infectious_rate;

	gamma_draw_list( model->asymptomatic_time_draws, 	N_DRAW_LIST, params->mean_asymptomatic_to_recovery, params->sd_asymptomatic_to_recovery );
	gamma_draw_list( model->symptomatic_time_draws, 	N_DRAW_LIST, params->mean_time_to_symptoms, params->sd_time_to_symptoms );
	gamma_draw_list( model->recovered_time_draws,   	N_DRAW_LIST, params->mean_time_to_recover,  params->sd_time_to_recover );
	gamma_draw_list( model->death_time_draws,       	N_DRAW_LIST, params->mean_time_to_death,    params->sd_time_to_death );
	bernoulli_draw_list( model->hospitalised_time_draws, N_DRAW_LIST, params->mean_time_to_hospital );

	infectious_rate = params->infectious_rate / params->mean_daily_interactions;
	gamma_rate_curve( model->presymptomatic.infectious_curve, MAX_INFECTIOUS_PERIOD, params->mean_infectious_period,
					  params->sd_infectious_period, infectious_rate );

	gamma_rate_curve( model->asymptomatic.infectious_curve, MAX_INFECTIOUS_PERIOD, params->mean_infectious_period,
				      params->sd_infectious_period, infectious_rate * params->asymptomatic_infectious_factor);

	gamma_rate_curve( model->symptomatic.infectious_curve, MAX_INFECTIOUS_PERIOD, params->mean_infectious_period,
				      params->sd_infectious_period, infectious_rate );

	gamma_rate_curve( model->hospitalised.infectious_curve, MAX_INFECTIOUS_PERIOD, params->mean_infectious_period,
					  params->sd_infectious_period, infectious_rate );
}

/*****************************************************************************************
*  Name:		new_event
*  Description: gets a new event tag
*  Returns:		void
******************************************************************************************/
event* new_event( model *model )
{
	event *event = model->next_event;

	model->next_event       = event->next;
	model->next_event->last = event->last;
	event->last->next       = model->next_event;

	return event;
}

/*****************************************************************************************
*  Name:		transmit_virus_by_type
*  Description: Transmits virus over the interaction network for a type of
*  				infected people
*  Returns:		void
******************************************************************************************/
void transmit_virus_by_type(
	model *model,
	event_list *list
)
{
	long idx, jdx, n_infected;
	int day, n_interaction;
	double hazard_rate;
	event *event, *next_event;
	interaction *interaction;
	individual *infector;

	for( day = model->time-1; day >= max( 0, model->time - MAX_INFECTIOUS_PERIOD ); day-- )
	{
		hazard_rate = list->infectious_curve[ model->time- 1 - day ];
		n_infected  = list->n_daily_current[ day];
		next_event  = list->events[ day ];

		for( idx = 0; idx < n_infected; idx++ )
		{
			event      = next_event;
			next_event = event->next;

			infector      = event->individual;
			n_interaction = infector->n_interactions[ model->interaction_day_idx ];
			interaction   = infector->interactions[ model->interaction_day_idx ];

			for( jdx = 0; jdx < n_interaction; jdx++ )
			{
				if( interaction->individual->status == UNINFECTED )
				{
					interaction->individual->hazard -= hazard_rate;
					if( interaction->individual->hazard < 0 )
						new_infection( model, interaction->individual, infector );
				}
				interaction = interaction->next;
			}
		}
	}
}

/*****************************************************************************************
*  Name:		transmit_virus
*  Description: Transmits virus over the interaction network
*
*  				Transmission by groups depending upon disease status.
*  				Note that quarantine is not a disease status and they will either
*  				be presymptomatic/symptomatic/asymptomatic and quarantining is
*  				modelled by reducing the number of interactions in the network.
*
*  Returns:		void
******************************************************************************************/
void transmit_virus( model *model )
{
	transmit_virus_by_type( model, &(model->presymptomatic) );
	transmit_virus_by_type( model, &(model->symptomatic) );
	transmit_virus_by_type( model, &(model->asymptomatic) );
	transmit_virus_by_type( model, &(model->hospitalised) );
}

/*****************************************************************************************
*  Name:		transition_to_symptomatic
*  Description: Transitions infected who are due to become symptomatic
*  Returns:		void
******************************************************************************************/
void transition_to_symptomatic( model *model )
{
	long idx, n_infected;
	int time_hospital;
	event *event, *next_event;
	individual *indiv;

	n_infected = model->symptomatic.n_daily_current[ model->time ];
	next_event = model->symptomatic.events[ model->time ];

	for( idx = 0; idx < n_infected; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		indiv->status = SYMPTOMATIC;
		remove_event_from_event_list( &(model->presymptomatic), indiv->current_event, model, indiv->time_infected );

		time_hospital            = model->time + sample_draw_list( model->hospitalised_time_draws );
		indiv->time_hospitalised = time_hospital;
		indiv->next_event_type   = HOSPITALISED;
		indiv->current_event     = event;
		add_individual_to_event_list( &(model->hospitalised), indiv, time_hospital, model );
	}
}

/*****************************************************************************************
*  Name:		quarantine_contracts
*  Description: Quarantine contacts
*  Returns:		void
******************************************************************************************/
void quarantine_contacts( model *model, individual *indiv )
{
	interaction *inter;
	individual *contact;
	int idx, ddx, day, n_contacts;
	int time_event;

	day = model->interaction_day_idx;
	for( ddx = 0; ddx < model->params.quarantine_days; ddx++ )
	{
		n_contacts = indiv->n_interactions[day];
		time_event = model->time + max( model->params.test_insensititve_period - ddx, 1 );

		if( n_contacts > 0 )
		{
			inter = indiv->interactions[day];
			for( idx = 1; idx < n_contacts; idx++ )
			{
				contact = inter->individual;
				if( contact->status != HOSPITALISED && contact->status != DEATH && !contact->quarantined )
				{
					if( gsl_ran_bernoulli( rng, model->params.quarantine_fraction ) )
					{
						set_quarantine_status( contact, &(model->params), model->time, TRUE );
						contact->quarantine_event = add_individual_to_event_list( &(model->quarantined), contact, model->time, model );
						add_individual_to_event_list( &(model->test_take), contact, time_event, model );
					}
				}
				inter = inter->next;
			}
		}
		day = ifelse( day == 0, model->params.days_of_interactions -1, day-1 );
	}

}

/*****************************************************************************************
*  Name:		transition_to_hospitalised
*  Description: Transitions symptomatic individual to hospital
*  Returns:		void
******************************************************************************************/
void transition_to_hospitalised( model *model )
{
	long idx, n_hospitalised;
	double time_event;
	event *event, *next_event;
	individual *indiv;

	n_hospitalised = model->hospitalised.n_daily_current[ model->time ];
	next_event     = model->hospitalised.events[ model->time ];

	for( idx = 0; idx < n_hospitalised; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		if( indiv->quarantined )
		{
			remove_event_from_event_list( &(model->quarantined), indiv->quarantine_event, model, indiv->time_quarantined );
			set_quarantine_status( indiv, &(model->params), model->time, FALSE );
		}

		set_hospitalised( indiv, &(model->params), model->time );
		remove_event_from_event_list( &(model->symptomatic), indiv->current_event, model, indiv->time_symptomatic );

		indiv->current_event = event;
		if( gsl_ran_bernoulli( rng, model->params.cfr ) )
		{
			time_event             = model->time + sample_draw_list( model->death_time_draws );
			indiv->time_death      = time_event;
			indiv->next_event_type = DEATH;
			add_individual_to_event_list( &(model->death), indiv, time_event, model );
		}
		else
		{
			time_event             = model->time + sample_draw_list( model->recovered_time_draws );
			indiv->time_recovered  = time_event;
			indiv->next_event_type = RECOVERED;
			add_individual_to_event_list( &(model->recovered), indiv, time_event, model );
		};

		quarantine_contacts( model, indiv );
	}
}

/*****************************************************************************************
*  Name:		transition_to_recovered
*  Description: Transitions hospitalised and asymptomatic to recovered
*  Returns:		void
******************************************************************************************/
void transition_to_recovered( model *model )
{
	long idx, n_recovered;
	event *event, *next_event;
	individual *indiv;

	n_recovered = model->recovered.n_daily_current[ model->time ];
	next_event  = model->recovered.events[ model->time ];
	for( idx = 0; idx < n_recovered; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		if( indiv->status == HOSPITALISED )
			remove_event_from_event_list( &(model->hospitalised), indiv->current_event, model, indiv->time_hospitalised );
		else
			remove_event_from_event_list( &(model->asymptomatic), indiv->current_event, model, indiv->time_asymptomatic );
		set_recovered( indiv, &(model->params), model->time );
	}
}

/*****************************************************************************************
*  Name:		transition_to_death
*  Description: Transitions hospitalised to death
*  Returns:		void
******************************************************************************************/
void transition_to_death( model *model )
{
	long idx, n_death;
	event *event, *next_event;
	individual *indiv;

	n_death    = model->death.n_daily_current[ model->time ];
	next_event = model->death.events[ model->time ];
	for( idx = 0; idx < n_death; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		remove_event_from_event_list( &(model->hospitalised), indiv->current_event, model, indiv->time_hospitalised );
		set_dead( indiv, model->time );
	}
}

/*****************************************************************************************
*  Name:		release_from_quarantine
*  Description: Release those held in quarantine
*  Returns:		void
******************************************************************************************/
void release_from_quarantine( model *model )
{
	long idx, n_quarantined;
	event *event, *next_event;
	individual *indiv;

	n_quarantined = model->quarantine_release.n_daily_current[ model->time ];
	next_event    = model->quarantine_release.events[ model->time ];

	for( idx = 0; idx < n_quarantined; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		if( indiv->quarantined )
		{
			remove_event_from_event_list( &(model->quarantined), indiv->quarantine_event, model, indiv->time_quarantined );
			remove_event_from_event_list( &(model->quarantine_release), event, model, model->time );
			set_quarantine_status( indiv, &(model->params), model->time, FALSE );
		}
	}
}

/*****************************************************************************************
*  Name:		quarantined_test_take
*  Description: Take a test
*  Returns:		void
******************************************************************************************/
void quarantined_test_take( model *model )
{
	long idx, n_test_take;
	event *event, *next_event;
	individual *indiv;

	n_test_take = model->test_take.n_daily_current[ model->time ];
	next_event  = model->test_take.events[ model->time ];

	for( idx = 0; idx < n_test_take; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		// add test
		if( indiv->status == UNINFECTED || indiv->status == RECOVERED )
			indiv->quarantine_test_result = FALSE;
		else
			indiv->quarantine_test_result = TRUE;

		add_individual_to_event_list( &(model->test_result), indiv, model->time + model->params.test_result_wait, model );
		remove_event_from_event_list( &(model->test_take), event, model, model->time );
	}
}

/*****************************************************************************************
*  Name:		quarantined_test_result
*  Description: Receive test result
*  Returns:		void
******************************************************************************************/
void quarantined_test_result( model *model )
{
	long idx, n_test_result;
	event *event, *next_event;
	individual *indiv;

	n_test_result = model->test_result.n_daily_current[ model->time ];
	next_event    = model->test_result.events[ model->time ];

	for( idx = 0; idx < n_test_result; idx++ )
	{
		event      = next_event;
		next_event = event->next;
		indiv      = event->individual;

		if( indiv->quarantine_test_result == FALSE )
			add_individual_to_event_list( &(model->quarantine_release), indiv, model->time, model );
		else
		{
			add_individual_to_event_list( &(model->quarantine_release), indiv, model->time + 14, model );
			quarantine_contacts( model, indiv );
		}

		remove_event_from_event_list( &(model->test_result), event, model, model->time );
	}
}

/*****************************************************************************************
*  Name:		add_indiv_to_event_list
*  Description: adds an individual to an event list at a particular time
*
*  Arguments:	list:	pointer to the event list
*  				indiv:	pointer to the individual
*  				time:	time of the event (int)
*  				model:	pointer to the model
*
*  Returns:		a pointer to the newly added event
******************************************************************************************/
event* add_individual_to_event_list(
	event_list *list,
	individual *indiv,
	int time,
	model *model
)
{
	event *event        = new_event( model );
	event->individual   = indiv;

	if( list->n_daily_current[time] > 1  )
	{
		list->events[ time ]->last = event;
		event->next  = list->events[ time ];
	}
	else
	{
		if( list->n_daily_current[time] == 1 )
		{
			list->events[ time ]->next = event;
			list->events[ time ]->last = event;
			event->next = list->events[ time ];
			event->last = list->events[ time ];
		}
	}

	list->events[time ] = event;
	list->n_daily[time]++;
	list->n_daily_current[time]++;

	return event;
}

/*****************************************************************************************
*  Name:		remove_event_from_event_list
*  Description: removes an event from an list at a particular time
*
*  Arguments:	list:	pointer to the event list
*  				event:	pointer to the event
*  				time:	time of the event (int)
*
*  Returns:		a pointer to the newly added event
******************************************************************************************/
void remove_event_from_event_list(
	event_list *list,
	event *event,
	model *model,
	int time
)
{
	if( list->n_daily_current[ time ] > 1 )
	{
		if( event != list->events[ time ] )
		{
			event->last->next = event->next;
			event->next->last = event->last;
		}
		else
			list->events[ time ] = event->next;
	}
	else
		list->events[time] = NULL;

	model->next_event->last->next = event;
	event->last = model->next_event->last;
	event->next = model->next_event;
	model->next_event->last = event;

	list->n_current--;
	list->n_daily_current[ time ]--;
}

/*****************************************************************************************
*  Name:		update_event_list_counters
*  Description: updates the event list counters, called at the end of a time step
*  Returns:		void
******************************************************************************************/
void update_event_list_counters( event_list *list, model *model )
{
	list->n_current += list->n_daily_current[ model->time ];
	list->n_total	+= list->n_daily[ model->time ];
}

/*****************************************************************************************
*  Name:		new_infection
*  Description: infects a new individual
*  Returns:		void
******************************************************************************************/
void new_infection(
	model *model,
	individual *infected,
	individual *infector
)
{
	int time_symptoms, time_recovery;
	infected->infector = infector;

	if( gsl_ran_bernoulli( rng, model->params.fraction_asymptomatic ) )
	{
		infected->status            = ASYMPTOMATIC;
		infected->time_infected     = model->time;
		infected->time_asymptomatic = model->time;
		infected->current_event = add_individual_to_event_list( &(model->asymptomatic), infected, model->time, model );

		time_recovery = model->time + sample_draw_list( model->asymptomatic_time_draws );
		infected->time_recovered = time_recovery;
		infected->next_event_type  = RECOVERED;
		add_individual_to_event_list( &(model->recovered), infected, time_recovery, model );
	}
	else
	{
		infected->status        = PRESYMPTOMATIC;
		infected->time_infected = model->time;
		infected->current_event = add_individual_to_event_list( &(model->presymptomatic), infected, model->time, model );

		time_symptoms = model->time + sample_draw_list( model->symptomatic_time_draws );
		infected->time_symptomatic = time_symptoms;
		infected->next_event_type  = SYMPTOMATIC;
		add_individual_to_event_list( &(model->symptomatic), infected, time_symptoms, model );
	}
}

/*****************************************************************************************
*  Name:		set_up_event_list
*  Description: sets up an event_list
*  Returns:		void
******************************************************************************************/
void set_up_event_list( event_list *list, parameters *params )
{
	int day;

	list->n_current = 0;
	list->n_total   = 0;
	for( day = 0; day < params->end_time;day ++ )
	{
		list->n_daily[day] = 0;
		list->n_daily_current[day] = 0;
	}
}

/*****************************************************************************************
*  Name:		set_up_seed_infection
*  Description: sets up the initial population
*  Returns:		void
******************************************************************************************/
void set_up_seed_infection( model *model )
{
	parameters *params = &(model->params);
	int idx;
	unsigned long int person;

	for( idx = 0; idx < params->n_seed_infection; idx ++ )
	{
		person = gsl_rng_uniform_int( rng, params->n_total );
		new_infection( model, &(model->population[ person ]), &(model->population[ person ]) );
	}
	update_event_list_counters( &(model->presymptomatic), model );
	update_event_list_counters( &(model->asymptomatic), model );
}

/*****************************************************************************************
*  Name:		build_daily_newtork
*  Description: Builds a new interaction network
******************************************************************************************/
void build_daily_newtork( model *model )
{
	long idx, n_pos, person;
	int jdx;
	long *interactions = model->possible_interactions;
	long *all_idx      = &(model->interaction_idx);

	interaction *inter1, *inter2;
	individual *indiv1, *indiv2;

	int day = model->interaction_day_idx;
	for( idx = 0; idx < model->params.n_total; idx++ )
		model->population[ idx ].n_interactions[ day ] = 0;

	n_pos = 0;
	for( person = 0; person < model->params.n_total; person++ )
		for( jdx = 0; jdx < model->population[person].mean_interactions; jdx++ )
			interactions[n_pos++]=person;

	gsl_ran_shuffle( rng, interactions, n_pos, sizeof(long) );

	idx = 0;
	n_pos--;
	while( idx < n_pos )
	{
		if( interactions[ idx ] == interactions[ idx + 1 ] )
		{
			idx++;
			continue;
		}

		inter1 = &(model->interactions[ (*all_idx)++ ]);
		inter2 = &(model->interactions[ (*all_idx)++ ]);
		indiv1 = &(model->population[ interactions[ idx++ ] ] );
		indiv2 = &(model->population[ interactions[ idx++ ] ] );

		inter1->individual = indiv2;
		inter1->next       = indiv1->interactions[ day ];
		indiv1->interactions[ day ] = inter1;
		indiv1->n_interactions[ day ]++;

		inter2->individual = indiv1;
		inter2->next       = indiv2->interactions[ day ];
		indiv2->interactions[ day ] = inter2;
		indiv2->n_interactions[ day ]++;

		model->n_total_intereactions++;

		if( *all_idx > model->n_interactions )
			*all_idx = 0;
	}
};

/*****************************************************************************************
*  Name:		one_time_step
*  Description: Move the model through one time step
******************************************************************************************/
int one_time_step( model *model )
{
	(model->time)++;

	update_event_list_counters( &(model->symptomatic), model );
	update_event_list_counters( &(model->hospitalised), model );
	update_event_list_counters( &(model->recovered), model );
	update_event_list_counters( &(model->death), model );
	update_event_list_counters( &(model->test_take), model );
	update_event_list_counters( &(model->test_result), model );

	build_daily_newtork( model );
	transmit_virus( model );

	transition_to_symptomatic( model );
	transition_to_hospitalised( model );
	transition_to_recovered( model );
	transition_to_death( model );
	quarantined_test_take( model );
	quarantined_test_result( model );
	release_from_quarantine( model );

	update_event_list_counters( &(model->presymptomatic), model );
	update_event_list_counters( &(model->asymptomatic), model );
	update_event_list_counters( &(model->quarantined), model );
	model->n_quarantine_days += model->quarantined.n_current;

	ring_inc( model->interaction_day_idx, model->params.days_of_interactions );

	return 1;
};
