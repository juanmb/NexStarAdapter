#include <math.h>
#include <stdio.h>
#include "AstroLib.h"


float sx2rad(SxAngle sx)
{
    float rad = M_PI*((float)sx.deg + (float)sx.min/60.0 + (float)sx.sec/3600.0)/180.0;

    if (sx.sign)
        return 2*M_PI - rad;
    return rad;
}

void rad2sx(float rad, SxAngle *sx)
{
    float deg = rad*180/M_PI;
    if (deg > 180)
        deg = deg - 360;

    sx->sign = (deg < 0);
    deg = fabs(deg);
    float min = 60*(deg - int(deg));
    float sec = 60*(min - int(min));

    sx->deg = (uint8_t)deg;
    sx->min = (uint8_t)min;
    sx->sec = (uint8_t)sec;
}

void sx2string(SxAngle sx, char *buffer)
{
    sprintf(buffer, " %d:%02d:%02d", sx.deg, sx.min, sx.sec);
    if (sx.sign)
        buffer[0] = '-';
}

inline float normalize2pi(float h)
{
    int ih = (int)(h/2/M_PI);
    h = h - (float)ih*2*M_PI;
    return h < 0 ? h + 2*M_PI : h;
}

// Calculate the julian date of the midnight of a given date
float getJulianDate0(Date date)
{
    float a, b, c, d;
    float month = date.month;
    float year = date.year;

    if (month < 3) {
        month += 12;
        year -= 1;
    }

    a = (long)(year/100);
    b = 2 - a + (long)(a/4);
    c = (long)(365.25*(year + 4716));
    d = (long)(30.6001*(month + 1));

    return (float)date.day + b + c + d - 1524.5;
}

float getJulianDate(Date date)
{
    // TODO: apply DST?
    float h = ((float)date.hour - (float)date.offset +
            (float)date.min/60 + (float)date.sec/3600);
    return getJulianDate0(date) + h/24;
}

float getJ2000Date(Date date)
{
    return getJulianDate(date) - 2451545.0;
}


void dateFromJ2000(float jd, Date *date)
{
    //TODO
    date->year = 2017;
    date->month = 12;
    date->day = 16;
    date->hour = 11;
    date->min = 58;
    date->sec = 35;
    date->offset = 0;
    date->dst = 0;

    /*
    float q = jd + 0.5;
    int z = int(jd + 0.5);

    w = (int)((z - 1867216.25)/36524.25);
    b = 1524 + z + 1 + w - (int)(w/4);
    c = (int)((b - 122.1)/365.25);
    d = (int)(365.25*c);
    e = (int)((b - d)/30.6001);
    f = 30.6001*e;

    date->day = b - d - f + q - z;
    date->month = e - 1;
    if (date->month > 13)
        date->month -= 12;

    date->year = c - 4715;
    if (date->month < 3)
        date->year -= 1;
    */
}

// Calculate Greenwich Mean Sidereal Time in radians
// given a J2000-based julian date
// Reference: http://aa.usno.navy.mil/faq/docs/GAST.php
float getGMST(Date date)
{
    long jdx = getJulianDate0(date) - 2451544.5;
    float frac = ((float)date.hour - (float)date.offset +
            (float)date.min/60 + (float)date.sec/3600)/24 - 0.5;

    float gmst = 4.8949612127 + 0.0172027918*jdx + 6.3003880989849*frac;
    return normalize2pi(gmst);
}

// Calculate Local Sidereal Time in radians
// given a J2000-based julian date
float getLST(Date date, Location loc)
{
    float lst = getGMST(date) + loc.longitude;
    return normalize2pi(lst);
}

// Convert equatorial to horizontal coordinates
// Reference: http://www.geocities.jp/toshimi_taki/matrix/matrix.htm
void eqToHoriz(Location loc, EqHACoords eq, HorizCoords *hor)
{
    float x, y, z, x2, y2, z2;

    // Convert to rectangular coordinates
    x = cos(-eq.ha)*cos(eq.dec);
    y = sin(-eq.ha)*cos(eq.dec);
    z = sin(eq.dec);

    // Rotate the coordinate system along east-west axis
    float pz = M_PI/2 - loc.latitude;
    x2 = x*cos(pz) - z*sin(pz);
    y2 = y;
    z2 = x*sin(pz) + z*cos(pz);

    // Convert to horizontal coordinates
    hor->az = M_PI - atan2(y2, x2);
    hor->alt = asin(z2);
}

// Convert horizontal to equatorial coordinates
// Reference: http://www.geocities.jp/toshimi_taki/matrix/matrix.htm
void horizToEq(Location loc, HorizCoords hor, EqHACoords *eq)
{
    float x, y, z, x2, y2, z2;

    // Convert to rectangular coordinates
    x = cos(M_PI - hor.az)*cos(hor.alt);
    y = sin(M_PI - hor.az)*cos(hor.alt);
    z = sin(hor.alt);

    // Rotate the coordinate system along east-west axis
    float zp = loc.latitude - M_PI/2;
    x2 = x*cos(zp) - z*sin(zp);
    y2 = y;
    z2 = x*sin(zp) + z*cos(zp);

    // Convert to equatorial coordinates
    eq->ha = -atan2(y2, x2);
    eq->dec = asin(z2);

    if (eq->ha < 0)
        eq->ha += 2*M_PI;
}
