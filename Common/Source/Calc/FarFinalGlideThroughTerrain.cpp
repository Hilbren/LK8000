/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: FarFinalGlideThroughTerrain.cpp,v 1.1 2011/12/21 10:28:45 root Exp root $
*/

#include "externs.h"
#include "McReady.h"
#include "RasterTerrain.h"
#include "LKProcess.h"
#include "NavFunctions.h"




double FarFinalGlideThroughTerrain(const double this_bearing, 
				NMEA_INFO *Basic, 
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon,
                                const double max_range,
				bool *out_of_range,
				double myaltitude,
				double *TerrainBase)
{
  double irange = GlidePolar::MacCreadyAltitude(MACCREADY, 
						1.0, this_bearing, 
						Calculated->WindSpeed, 
						Calculated->WindBearing, 
						0, 0, true, 0);
  const double start_lat = Basic->Latitude;
  const double start_lon = Basic->Longitude;
  if (retlat && retlon) {
    *retlat = start_lat;
    *retlon = start_lon;
  }
  *out_of_range = false;

  if ((irange<=0.0)||(myaltitude<=0)) {
    // can't make progress in this direction at the current windspeed/mc
    return 0;
  }

  const double glide_max_range = myaltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  double lat, lon;
  double last_lat, last_lon;
  double h=0.0, dh=0.0;
//  int imax=0;
  double last_dh=0;
  double altitude;
 
  RasterTerrain::Lock();
  double retval = 0;
  int i=0;
  bool start_under = false;

  // calculate terrain rounding factor

  FindLatitudeLongitude(start_lat, start_lon, 0, 
                        glide_max_range/NUMFINALGLIDETERRAIN, &lat, &lon);

  double Xrounding = fabs(lon-start_lon)/2;
  double Yrounding = fabs(lat-start_lat)/2;
  RasterTerrain::SetTerrainRounding(Xrounding, Yrounding);

  lat = last_lat = start_lat;
  lon = last_lon = start_lon;

  altitude = myaltitude;
  h =  max(0, (int)RasterTerrain::GetTerrainHeight(lat, lon)); 
  if (h==TERRAIN_INVALID) h=0; //@ 101027 FIX
  dh = altitude - h - SAFETYALTITUDETERRAIN/10;
  last_dh = dh;
  if (dh<0) {
    start_under = true;
    // already below safety terrain height
    //    retval = 0;
    //    goto OnExit;
  }

  // find grid
  double dlat, dlon;

  FindLatitudeLongitude(lat, lon, this_bearing, glide_max_range, &dlat, &dlon);
  dlat -= start_lat;
  dlon -= start_lon;

  double f_scale = 1.0/NUMFINALGLIDETERRAIN;
  if ((max_range>0) && (max_range<glide_max_range)) {
    f_scale *= max_range/glide_max_range;
  }

  double delta_alt = -f_scale*myaltitude;

  dlat *= f_scale;
  dlon *= f_scale;

  for (i=1; i<=NUMFINALGLIDETERRAIN; i++) {
    double f;
    bool solution_found = false;
    double fi = i*f_scale;
    // fraction of glide_max_range

    if ((max_range>0)&&(fi>=1.0)) {
      // early exit
      *out_of_range = true;
      retval = max_range;
      goto OnExit;
    }

    if (start_under) {
      altitude += 2.0*delta_alt;
    } else {
      altitude += delta_alt;
    }

    // find lat, lon of point of interest

    lat += dlat;
    lon += dlon;

    // find height over terrain
    h =  max(0,(int)RasterTerrain::GetTerrainHeight(lat, lon)); 
    if (h==TERRAIN_INVALID) h=0;

    dh = altitude - h - SAFETYALTITUDETERRAIN/10;

    if (TerrainBase && (dh>0) && (h>0)) {
      *TerrainBase = min(*TerrainBase, h);
    }

    if (start_under) {
      if (dh>last_dh) {
	// better solution found, ok to continue...
	if (dh>0) {
	  // we've now found a terrain point above safety altitude,
	  // so consider rest of track to search for safety altitude
	  start_under = false;
	}
      } else {
	f= 0.0;
	solution_found = true;
      }
    } else if (dh<=0) {
      if ((dh<last_dh) && (last_dh>0)) {
        BUGSTOP_LKASSERT((dh-last_dh)!=0);

	if ( dh-last_dh==0 ) {
		f = 0.0;
	} else
        f = max(0.0,min(1.0,(-last_dh)/(dh-last_dh)));
      } else {
	f = 0.0;
      }
      solution_found = true;
    }
    if (solution_found) {
      double distance;
      lat = last_lat*(1.0-f)+lat*f;
      lon = last_lon*(1.0-f)+lon*f;
      if (retlat && retlon) {
        *retlat = lat;
        *retlon = lon;
      }
      DistanceBearing(start_lat, start_lon, lat, lon, &distance, NULL);
      retval = distance;
      goto OnExit;
    }
    last_dh = dh;
    last_lat = lat;
    last_lon = lon;
  }

  *out_of_range = true;
  retval = glide_max_range;

 OnExit:
  RasterTerrain::Unlock();
  return retval;
}


