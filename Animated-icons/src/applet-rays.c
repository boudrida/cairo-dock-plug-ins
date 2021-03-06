/**
* This file is a part of the Cairo-Dock project
*
* Copyright : (C) see the 'copyright' file.
* E-mail    : see the 'copyright' file.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "applet-struct.h"
#include "applet-rays.h"

CairoParticleSystem *cd_animations_init_rays (Icon *pIcon, CairoDock *pDock, double dt)
{
	if (myData.iRaysTexture == 0)
		myData.iRaysTexture = cd_animations_load_rays_texture ();
	CairoParticleSystem *pRaysParticleSystem = cairo_dock_create_particle_system (myConfig.iNbRaysParticles,
		myData.iRaysTexture,
		pIcon->fWidth,
		pDock->container.bIsHorizontal ? pIcon->image.iHeight : pIcon->image.iWidth);
	pRaysParticleSystem->dt = dt;
	pRaysParticleSystem->bDirectionUp = (pDock->container.bIsHorizontal ? pDock->container.bDirectionUp : ! pDock->container.bDirectionUp);
	pRaysParticleSystem->bAddLuminance = TRUE;
	
	double a = myConfig.fRaysParticleSpeed;
	static double epsilon = 0.1;
	double r = myConfig.iRaysParticleSize;
	double fBlend, fPhase;
	double vmax = 1. / myConfig.iSpotDuration;
	CairoParticle *p;
	int i;
	for (i = 0; i < myConfig.iNbRaysParticles; i ++)
	{
		p = &(pRaysParticleSystem->pParticles[i]);
		
		fPhase = (2 * g_random_double () - 1) * G_PI;
		p->x = .9 * sin (fPhase);
		p->z = cos (fPhase);
		p->fHeight = r*(p->z + 2)/3;
		p->y = ((1 - p->z) * CD_ANIMATIONS_SPOT_HEIGHT + p->fHeight/2) / pRaysParticleSystem->fHeight;
		p->fWidth = (p->z + 2)/2;
		
		
		p->vx = .25 * p->x / myConfig.iSpotDuration * dt;
		p->vy = a * vmax * ((p->z + 1)/2 * g_random_double () + epsilon) * dt;
		p->iInitialLife = MIN (1./ p->vy, ceil (myConfig.iSpotDuration / dt));
		p->iLife = p->iInitialLife;
		
		if (myConfig.bMysticalRays)
		{
			p->color[0] = g_random_double ();
			p->color[1] = g_random_double ();
			p->color[2] = g_random_double ();
		}
		else
		{
			fBlend = g_random_double ();
			p->color[0] = fBlend * myConfig.pRaysColor1[0] + (1 - fBlend) * myConfig.pRaysColor2[0];
			p->color[1] = fBlend * myConfig.pRaysColor1[1] + (1 - fBlend) * myConfig.pRaysColor2[1];
			p->color[2] = fBlend * myConfig.pRaysColor1[2] + (1 - fBlend) * myConfig.pRaysColor2[2];
		}
		p->color[3] = 1.;
		
		p->fSizeFactor = .3;
		p->fResizeSpeed = .1;
	}
	
	return pRaysParticleSystem;
}



void cd_animations_rewind_rays_particle (CairoParticle *p, double dt, double fHeight)
{
	static double epsilon = 0.1;
	double a = myConfig.fRaysParticleSpeed/3;
	double r = myConfig.iRaysParticleSize;
	double vmax = 1. / myConfig.iSpotDuration;
	double fPhase = (2 * g_random_double () - 1) * G_PI;
	//p->x = 2 * g_random_double () - 1;
	p->x = .9 * sin (fPhase);
	p->z = cos (fPhase);
	p->fHeight = r*(p->z + 2)/3;
	p->y = ((1 - p->z) * CD_ANIMATIONS_SPOT_HEIGHT + p->fHeight/2) / fHeight;
	p->vy = a * vmax * ((p->z + 1)/2 * g_random_double () + epsilon) * dt;
	p->vx = .25 * p->x / myConfig.iSpotDuration * dt;
	
	p->iInitialLife = MIN (1./ p->vy, ceil (myConfig.iSpotDuration / dt));
	p->iLife = p->iInitialLife;
	
	p->fSizeFactor = .3;
}


gboolean cd_animations_update_rays_system (CairoParticleSystem *pParticleSystem, gboolean bContinue)
{
	gboolean bAllParticlesEnded = TRUE;
	CairoParticle *p;
	int i;
	for (i = 0; i < pParticleSystem->iNbParticles; i ++)
	{
		p = &(pParticleSystem->pParticles[i]);
		
		p->x += p->vx;
		p->y += p->vy;
		p->color[3] = 1.*p->iLife / p->iInitialLife;
		if (p->fSizeFactor < 1)
			p->fSizeFactor += p->fResizeSpeed;
		if (p->iLife > 0)
		{
			p->iLife --;
			if (bContinue && p->iLife == 0)
			{
				cd_animations_rewind_rays_particle (p, pParticleSystem->dt, pParticleSystem->fHeight);
			}
			if (bAllParticlesEnded && p->iLife != 0)
				bAllParticlesEnded = FALSE;
		}
		else if (bContinue)
			cd_animations_rewind_rays_particle (p, pParticleSystem->dt, pParticleSystem->fHeight);
	}
	return ! bAllParticlesEnded;
}
