#include <event.h>

void event_init( event *e )
{ *e = 0; }

void event_trigger( event *e )
{ *e = 1; }

int event_check( event *e )
{
  if ( *e )
  {
    *e = 0;
	return 1;
  }

  return 0;
}

int event_read( event *e )
{
  return *e;
}

//PAS BON (ou alors section critique nécessaire) :
//int event_check( event *e )
//{
//	event temp_e = *e;
//	*e = 0;
//	return temp_e;
//}

// Exemple:
// Si *e vaut 0 au début
// temp_e = *e donc temp_e vaut 0
// Puis une IT arrive ---> *e vaut 1
// Puis *e = 0 donc on écrase le 1 avec un 0 dans *e
// On renvoie temp_e (0)
// On a perdu l'IT...
