/*
  ******************************************************************************
  * @file    event_dispatcher.h
  * @author  A.Alborov
  ******************************************************************************
*/

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef TEMPLATE_EVENT_NAME
	#ifdef TEMPLATE_BIND_BUFFER_SIZE
	 
	#include "stdint.h"
	#include "stdbool.h"
	#include "templates.h"

		#ifdef TEMPLATE_EVENT_ARG_NUM
		
		#if TEMPLATE_EVENT_ARG_NUM == 1

		typedef void (*TEMPLATE(type,TEMPLATE_EVENT_NAME))(TEMPLATE_EVENT_ARG_1 a1);

		TEMPLATE(type,TEMPLATE_EVENT_NAME)TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[TEMPLATE_BIND_BUFFER_SIZE];
		uint8_t TEMPLATE(TEMPLATE_EVENT_NAME,num_events) = 0;

		void TEMPLATE_EVENT_NAME(TEMPLATE_EVENT_ARG_1 a1)
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{	
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i](a1);
					lcnt++;
					if(lcnt >= num_events) return;
				}
			}
		}
		
		bool TEMPLATE(bind,TEMPLATE_EVENT_NAME)(void (*handler)(TEMPLATE_EVENT_ARG_1 a1))
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
					return true;
					
				if(lcnt >= num_events) break;	
			}
					
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = handler;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)++;
					return true;
				}
			}
			return false;
	  }
		
		bool TEMPLATE(unbind,TEMPLATE_EVENT_NAME)(void (*handler)(TEMPLATE_EVENT_ARG_1 a1))
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = 0;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)--;
					return true;
				}
				if(lcnt>=TEMPLATE(TEMPLATE_EVENT_NAME,num_events))return false;
			}
			return false;
	  }
		
		#undef TEMPLATE_EVENT_ARG_1
		#endif
		
		#if  TEMPLATE_EVENT_ARG_NUM == 2
		
		typedef void (*TEMPLATE(type,TEMPLATE_EVENT_NAME))(TEMPLATE_EVENT_ARG_1 a1, TEMPLATE_EVENT_ARG_2 a2);

		TEMPLATE(type,TEMPLATE_EVENT_NAME)TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[TEMPLATE_BIND_BUFFER_SIZE];
		uint8_t TEMPLATE(TEMPLATE_EVENT_NAME,num_events) = 0;

		void TEMPLATE_EVENT_NAME(TEMPLATE_EVENT_ARG_1 a1, TEMPLATE_EVENT_ARG_2 a2)
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i](a1,a2);
					lcnt++;
					if(lcnt >= TEMPLATE(TEMPLATE_EVENT_NAME,num_events)) return;
				}
			}
		}
		
		bool TEMPLATE(bind,TEMPLATE_EVENT_NAME)(void (*handler)(TEMPLATE_EVENT_ARG_1 a1, TEMPLATE_EVENT_ARG_2 a2))
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
					return true;
					
				if(lcnt >= TEMPLATE(TEMPLATE_EVENT_NAME,num_events)) break;	
			}
					
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = handler;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)++;
					return true;
				}
			}
			return false;
	  }
		
		bool TEMPLATE(unbind,TEMPLATE_EVENT_NAME)(void (*handler)(TEMPLATE_EVENT_ARG_1 a1, TEMPLATE_EVENT_ARG_2 a2))
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = 0;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)--;
					return true;
				}
				if(lcnt>=TEMPLATE(TEMPLATE_EVENT_NAME,num_events))return false;
			}
			return false;
	  }
		
		#undef TEMPLATE_EVENT_ARG_1
		#undef TEMPLATE_EVENT_ARG_2
		#endif
		#undef TEMPLATE_EVENT_ARG_NUM
		
		#else
		typedef void (*TEMPLATE(type,TEMPLATE_EVENT_NAME))();

		TEMPLATE(type,TEMPLATE_EVENT_NAME)TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[TEMPLATE_BIND_BUFFER_SIZE];
		uint8_t TEMPLATE(TEMPLATE_EVENT_NAME,num_events) = 0;

		void TEMPLATE_EVENT_NAME()
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]();
					lcnt++;
					if(lcnt >= TEMPLATE(TEMPLATE_EVENT_NAME,num_events)) return;
				}
			}
		}
		bool TEMPLATE(bind,TEMPLATE_EVENT_NAME)(void (*handler)())
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
					return true;
					
				if(lcnt >= TEMPLATE(TEMPLATE_EVENT_NAME,num_events)) break;	
			}
					
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== 0)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = handler;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)++;
					return true;
				}
			}
			return false;
	  }
		
		bool TEMPLATE(unbind,TEMPLATE_EVENT_NAME)(void (*handler)())
		{
			uint8_t lcnt = 0;
			for(uint8_t i = 0; i<TEMPLATE_BIND_BUFFER_SIZE; i++)
			{
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] != 0)
					lcnt++;
				
				if(TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i]== handler)
				{
					TEMPLATE(TEMPLATE_EVENT_NAME,bind_events)[i] = 0;
					TEMPLATE(TEMPLATE_EVENT_NAME,num_events)--;
					return true;
				}
				if(lcnt>=TEMPLATE(TEMPLATE_EVENT_NAME,num_events))return false;
			}
			return false;
	  }
		#endif 
		

	#undef TEMPLATE_BIND_BUFFER_SIZE  
	#endif 
#undef TEMPLATE_EVENT_NAME
#endif 

#ifdef __cplusplus
}
#endif
