// ################################################################################################
// Script API
// ################################################################################################
extern "C" {
#include "../lua-5.3.5/lua.h"
#include "../lua-5.3.5/lualib.h"
#include "../lua-5.3.5/lauxlib.h"
}

// ================================================
real gpixelsize = 1;
static int rnd(lua_State* L)
{
	if (lua_gettop(L) == 2)
	{
		real min = lua_tonumber(L, 1);
		real max = lua_tonumber(L, 2);

		lua_pushnumber(L, rrnd(min, max));
	}
	else
		lua_pushnumber(L, rrnd(0, 1));
	return 1;
}
static int bnd(lua_State* L)
{

	real min = lua_tonumber(L, 1);
	real max = lua_tonumber(L, 2);
	real a = lua_tonumber(L, 3);

	real pw = 1;
	if (lua_gettop(L) >= 4)
		pw = lua_tonumber(L, 4);

	lua_pushnumber(L, blend(min, max, a, pw));
	return 1;
}
static int atan2(lua_State* L)
{
	real y = lua_tonumber(L, 1);
	real x = lua_tonumber(L, 2);

	lua_pushnumber(L, atan2(y, x));
	return 1;
}
static int pow(lua_State* L)
{
	real x = lua_tonumber(L, 1);
	real y = lua_tonumber(L, 2);

	lua_pushnumber(L, pow(x, y));
	return 1;
}
static int print(lua_State* L)
{
	std::string str = lua_tostring(L, 1);
	PRINT(str);

	return 0;
}
static int pixsize(lua_State* L)
{
	real sz = lua_tonumber(L, 1);
	gpixelsize = sz;
	return 0;
}

static int pixel(lua_State* L)
{
	real x = lua_tonumber(L, 1);
	real y = lua_tonumber(L, 2);
	real z = 0;
	if (lua_gettop(L) >= 3)
		z = lua_tonumber(L, 3);
	if (gpixelsize > 1)
	{
		point(vec(x, y, z), gpixelsize, color);
	}
	else
		pixel(vec(x, y, z), color);
	return 0;
}
static int pixeli(lua_State* L)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	if (gpixelsize > 1)
	{
		pointi(x, y, gpixelsize, color);
	}
	else
		pixel(x, y, color);
	return 0;
}
static int triang(lua_State* L)
{
	real x1 = lua_tonumber(L, 1);
	real y1 = lua_tonumber(L, 2);
	real z1 = lua_tonumber(L, 3);

	real x2 = lua_tonumber(L, 1 + 3);
	real y2 = lua_tonumber(L, 2 + 3);
	real z2 = lua_tonumber(L, 3 + 3);

	real x3 = lua_tonumber(L, 1 + 6);
	real y3 = lua_tonumber(L, 2 + 6);
	real z3 = lua_tonumber(L, 3 + 6);

	triang0(vec(x1, y1, z1), vec(x2, y2, z2), vec(x3, y3, z3));

	return 0;
}
static int pset(lua_State* L)
{
	real x = lua_tonumber(L, 1);
	real y = lua_tonumber(L, 2);
	real z = 0;
	if (lua_gettop(L) >= 3)
		z = lua_tonumber(L, 3);
	if (gpixelsize > 1)
	{
		PSET(vec(x, y, z), color, gpixelsize);
	}
	else
		PSET(vec(x, y, z), color);
	return 0;
}
// ------------------------------------------------
// 渲染
// ------------------------------------------------
extern void rayrender();
static int rayrender(lua_State* L)
{
	rayrender();
	return 0;
}

extern void scanline::scanlinerender();
static int scanlinerender(lua_State* L)
{
	int n = lua_gettop(L);
	for (int i = 0; i < n; i++)
		luaparam[i] = lua_tonumber(L, 1 + i);

	scanline::scanlinerender();
	return 0;
}

// ------------------------------------------------
// 贝壳形态学
// ------------------------------------------------
std::vector<VECLIST> estack;
std::vector<vec>	verstack;
std::vector<vec>	dirstack;
std::vector<real>	weightlist;

extern void edgeax(const VECLIST& e, vec& ux, vec& uy, vec& uz);
extern void edgeax(const VECLIST& e, vec& ux, vec& uy, const vec& uz);
struct coord_t
{
	vec ux, uy, uz;
	coord_t(const VECLIST& e)
	{
		edgeax(e, ux, uy, uz);
		//PRINTVEC("ux=", ux);
		//PRINTVEC("uy=", uy);
		//PRINTVEC("uz=", uz);
	}
	coord_t(crvec _ux, crvec _uy, crvec _uz)
	{
		ux = _ux;
		uy = _uy;
		uz = _uz;
	}
};
std::vector<coord_t>	coordstack;

real gwtimes = 1;
bool bmirror = false;
vec gCurPos = vec::ZERO;
vec gUpperDir = vec::UY;
int growtime = 0;
int growpos = -1;

// -------------------------------------------------
void edgeax(const VECLIST& e, vec& ux, vec& uy, vec& uz)
{
	vec o = getedgecenter(e);
	uz = (o - e[0].p).normcopy();
	uy = getedgenorm(estack.back());
	ux = uz.cross(uy);
}
void edgeax2(VECLIST& e, vec& ux, vec& uy, vec& uz)
{
	if (coordstack.empty())
		edgeax(e, ux, uy, uz);
	else
	{
		ux = coordstack.back().ux;
		uy = coordstack.back().uy;
		uz = coordstack.back().uz;
	}
}
vec getedgenorm2(const VECLIST& e)
{
	if (coordstack.empty())
		return getedgenorm(e);
	return coordstack.back().uy;
}
void extrudeedge2(VECLIST& e1, real d)
{
	if (coordstack.empty())
		return extrudeedge(e1, d);

	if (e1.size() < 3)
		return;
	vec norm = getedgenorm2(e1);
	vec dv = norm * d;
	if (weightlist.size() == e1.size())
	{
		for (int i = 0; i < e1.size(); i++)
		{
			e1[i].p += dv * weightlist[i];
			e1[i].ind = -1;
		}
	}
	else
	{
		for (int i = 0; i < e1.size(); i++)
		{
			e1[i].p += dv;
			e1[i].ind = -1;
		}
	}
}
void rotedge2(const VECLIST& e, real ang, const vec& ax, VECLIST& eo)
{
	vec o = getedgecenter(e);
	for (int i = 0; i < e.size(); i++)
	{
		if (bmirror)
		{
			eo.PUSH((e[i].p - o).rotcopy(i < e.size() / 2 ? ang : -ang, ax) + o);
		}
		else
			eo.PUSH((e[i].p - o).rotcopy(ang, ax) + o);
	}
}
// ------------------------------------------------
static int pushe(lua_State* L)
{
	if (estack.empty())
		estack.push_back(VECLIST());
	else
		estack.push_back(estack.back());

	return 0;
}
static int pope(lua_State* L)
{
	int n = 1;
	if (lua_gettop(L) == 1)
		n = lua_tonumber(L, 1);
	for (int i = 0; i < n; i++)
		estack.pop_back();

	return 0;
}
static int closeedge(lua_State* L)
{
	if (lua_gettop(L) == 1)
	{
		bclosed = lua_tonumber(L, 1);
	}
	else
		closeedge(estack.back());


	return 0;
}
static int setpos(lua_State* L)
{
	gCurPos.x = lua_tonumber(L, 1);
	gCurPos.y = lua_tonumber(L, 2);
	gCurPos.z = lua_tonumber(L, 3);

	return 0;
}
static int setepos(lua_State* L)
{
	gCurPos = getedgecenter(estack.back());
	return 0;
}
static int pushcoord(lua_State* L)
{
	coordstack.push_back(coordstack.back());
	return 0;
}
static int calccoord(lua_State* L)
{
	if (lua_gettop(L) == 6)
	{
		vec ux, uy, uz;
		ux.x = lua_tonumber(L, 1);
		ux.y = lua_tonumber(L, 2);
		ux.z = lua_tonumber(L, 3);
		ux.norm();

		uy.x = lua_tonumber(L, 4);
		uy.y = lua_tonumber(L, 5);
		uy.z = lua_tonumber(L, 6);
		uy.norm();

		uz = uy.cross(ux).normcopy();
	}
	else
		coordstack.push_back(coord_t(estack.back()));
	return 0;
}
static int coorduz(lua_State* L)
{
	real s = lua_tonumber(L, 1);
	int i = s > 1 ? s : s * (estack.back().size() - 1);
	VECLIST& e = estack.back();
	vec o = (e[0].p == e[e.size() - 1].p) ? getedgecenter(e) : (e[0].p + e[e.size() - 1].p) / 2;

	vec uz = estack.back()[i].p - o;
	uz.norm();
	coordstack.back().uz = uz;
	coordstack.back().uy = getedgenorm2(estack.back());
	coordstack.back().ux = uz.cross(coordstack.back().uy).normcopy();
	coordstack.back().uy = coordstack.back().ux.cross(coordstack.back().uz);

	return 0;
}
static int coordux(lua_State* L)
{
	real s = lua_tonumber(L, 1);
	int i = s > 1 ? s : s * (estack.back().size() - 1);
	VECLIST& e = estack.back();
	vec o = (e[0].p == e[e.size() - 1].p) ? getedgecenter(e) : (e[0].p + e[e.size() - 1].p) / 2;

	vec ux = estack.back()[i].p - o;
	ux.norm();
	coordstack.back().ux = ux;
	coordstack.back().uy = getedgenorm2(estack.back());
	coordstack.back().uz = coordstack.back().uy.cross(ux).normcopy();
	coordstack.back().uy = coordstack.back().ux.cross(coordstack.back().uz);

	return 0;
}
static int popcoord(lua_State* L)
{
	int n = 1;
	if (lua_gettop(L) == 1)
		n = lua_tonumber(L, 1);
	for (int i = 0; i < n; i++)
		coordstack.pop_back();
	return 0;
}
static int mirror(lua_State* L)
{
	bmirror = lua_tonumber(L, 1);
	return 0;
}
static int setdir(lua_State* L)
{
	vec dir = gUpperDir;
	if (lua_gettop(L) == 3)
	{
		dir.x = lua_tonumber(L, 1);
		dir.y = lua_tonumber(L, 2);
		dir.z = lua_tonumber(L, 3);
		dir.norm();
	}
	else if (lua_gettop(L) == 2)
	{
		int v1 = lua_tonumber(L, 1);
		int v2 = lua_tonumber(L, 2);
		dir = estack.back()[v2] - estack.back()[v1];
		dir.norm();
	}
	else
	{
		dir = getedgenorm(estack.back());
	}
	dirstack.push_back(dir);
	return 0;
}
static int popdir(lua_State* L)
{
	dirstack.pop_back();
	return 0;
}
static int roundedge(lua_State* L)
{
	float r = lua_tonumber(L, 1);
	float sig = lua_tonumber(L, 2);
	float ang0 = -PI / 2;
	if (lua_gettop(L) >= 3)
		ang0 = lua_tonumber(L, 3);
	vec up = gUpperDir;
	if (!dirstack.empty())
		up = dirstack.back();
	//PRINT("ang0 " << ang0)
	VECLIST e;
	roundedge(e, gCurPos, up, r, sig, ang0);
	estack.push_back(e);
	return 0;
}
static int arcedge(lua_State* L)
{
	float r = lua_tonumber(L, 1);
	int sig = lua_tonumber(L, 2);
	float ang0 = lua_tonumber(L, 3);
	float dang = lua_tonumber(L, 4);
	vec up = gUpperDir;
	if (!dirstack.empty())
		up = dirstack.back();
	vec dx, dy;
	v2vxvy(up, dx, dy);
	roundedgex(estack.back(), gCurPos, dx, dy, r, sig, dang, ang0);
	return 0;
}
static int curvedge(lua_State* L)
{
	float r = lua_tonumber(L, 1);
	float sig = lua_tonumber(L, 2);
	vec up = gUpperDir;
	if (!dirstack.empty())
		up = dirstack.back();
	vec side = vec::UZ.cross(up).normcopy();
	vec p1 = gCurPos - side * (r / 2);
	vec p2 = gCurPos + side * (r / 2);

	for (int i = 0; i < sig; i++)
	{
		vec p = blend(p1, p2, i / (sig - 1.0f));
		estack.back().push_back(p);
	}
	return 0;
}

static int colori(lua_State* L)
{
	int c = lua_tonumber(L, 1);
	color = c;
	return 0;
}

static int rgb(lua_State* L)
{
	int r = lua_tonumber(L, 1);
	int g = lua_tonumber(L, 2);
	int b = lua_tonumber(L, 3);
	color = RGB(r, g, b);
	return 0;
}
static int hsl(lua_State* L)
{
	real h = lua_tonumber(L, 1);
	real s = lua_tonumber(L, 2);
	real l = lua_tonumber(L, 3);

	COLOR_HSL thsl;
	thsl.hue = h * 255;
	thsl.saturation = s * 255;
	thsl.luminance = l * 255;
	color = HSLtoRGB(&thsl);

	return 0;
}

static int getver(lua_State* L)
{
	int ind = lua_tonumber(L, 1);

	if (ind < estack.back().size())
	{
		vec p = estack.back()[ind].p;

		verstack.push_back(p);
	}
	return 0;
}

static int pushver(lua_State* L)
{
	vec3 v(
		lua_tonumber(L, 1),
		lua_tonumber(L, 2),
		lua_tonumber(L, 3));
	verstack.push_back(v);
	return 0;
}
static int popver(lua_State* L)
{
	verstack.pop_back();
	return 0;
}
static int veradd(lua_State* L)
{
	int s1 = lua_tonumber(L, 1);
	int s2 = lua_tonumber(L, 2);

	vec3 v = verstack[verstack.size() + s1] +
		verstack[verstack.size() + s2];

	verstack.push_back(v);
	return 0;
}

static int extrudeedgeex(lua_State* L)
{
	float d = lua_tonumber(L, 1);
	float s = lua_tonumber(L, 2);
	extrudeedgeex(estack.back(), d, s);
	return 0;
}
static int extrudeedge(lua_State* L)
{
	float d = lua_tonumber(L, 1);

	VECLIST& e1 = estack.back();
	vec norm = getedgenorm2(e1);
	vec dv = norm * d;
	if (weightlist.size() == e1.size())
	{
		for (int i = 0; i < e1.size(); i++)
		{
			e1[i].p += dv * blend(0, 1, weightlist[i]);
			e1[i].ind = -1;
		}
	}
	else
	{
		for (int i = 0; i < e1.size(); i++)
		{
			e1[i].p += dv;
			e1[i].ind = -1;
		}
	}

	return 0;
}
static int moveedge(lua_State* L)
{
	if (lua_gettop(L) == 3)
	{
		float x = lua_tonumber(L, 1);
		float y = lua_tonumber(L, 2);
		float z = lua_tonumber(L, 3);

		VECLIST& e = estack.back();
		if (weightlist.size() == e.size())
		{
			VECLIST oe;
			moveedge(estack.back(), vec(x, y, z), oe);
			for (int i = 0; i < e.size() - 1; i++)
			{
				e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
			}
			e.back().p = e.front().p;
		}
		else
			moveedge(estack.back(), vec(x, y, z));
	}
	else if (lua_gettop(L) == 1)
	{
		float d = lua_tonumber(L, 1);
		VECLIST& e = estack.back();
		if (weightlist.size() == e.size())
		{
			VECLIST oe;
			moveedge(estack.back(), dirstack.back() * d, oe);
			for (int i = 0; i < e.size() - 1; i++)
			{
				e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
			}
			e.back().p = e.front().p;
		}
		else
			moveedge(estack.back(), dirstack.back() * d);
	}
	return 0;
}
static int rotedge(lua_State* L)
{
	float ang = lua_tonumber(L, 1);

	VECLIST& e = estack.back();
	VECLIST oe;
	rotedge2(e, ang, dirstack.back(), oe);

	if (weightlist.size() == e.size())
	{
		for (int i = 0; i < e.size(); i++)
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
	}
	else
		e = oe;

	return 0;
}
static int rotedgemirror(lua_State* L)
{
	VECLIST& e = estack.back();
	int len = e.size();

	//real fcenter = lua_tonumber(L, 1);
	//int center = fcenter > 1 ?	fcenter : len * fcenter;
	int center = len / 2;

	float ang1 = lua_tonumber(L, 1);
	float ang2 = lua_tonumber(L, 2);

	real pw = 1;
	if (lua_gettop(L) >= 3)
		pw = lua_tonumber(L, 3);


	//PRINT("len=" << len);
	VECLIST oe = e;
	{
		vec n = getedgenorm2(e);
		vec o = len % 2 == 0 ? (e[center - 1] + e[center]) / 2 : e[center];
		for (int offset = 0; offset < len / 2; offset++)
		{
			real ang = blend(ang1, ang2, (offset + 1) / real(len / 2), pw);
			int ind = (center - offset - 1);
			if (ind >= 0)
			{
				oe[ind] = ((e[ind].p - o).rotcopy(-ang, n) + o);
				//PRINT("rotedgemirror1 " << ind << " ang=" << ang);
			}
		}
		for (int offset = 1; offset <= len / 2; offset++)
		{
			real ang = blend(ang1, ang2, offset / real(len / 2), pw);
			int ind = (center + offset);
			if (ind < len)
			{
				oe[ind] = ((e[ind].p - o).rotcopy(ang, n) + o);
				//PRINT("rotedgemirror2 " << ind << " ang=" << ang);
			}
		}
	}
	if (weightlist.size() == len)
	{
		for (int i = 0; i < len; i++)
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
	}
	else
		e = oe;

	return 0;
}
static int rotonedge(lua_State* L)
{
	float ang = lua_tonumber(L, 1);
	float s = lua_tonumber(L, 2);

	VECLIST& e = estack.back();
	VECLIST& re = estack[estack.size() + s];
	if (re.size() == e.size())
	{
		VECLIST oe;
		vec ro = getedgecenter(re);
		vec n = getedgenorm2(re);
		for (int i = 0; i < e.size(); i++)
		{
			vec ax = (n.cross(re[i].p - ro)).normcopy();
			oe.PUSH((e[i] - re[i]).rotcopy(ang, ax) + re[i]);
		}

		if (weightlist.size() == e.size())
		{
			for (int i = 0; i < e.size(); i++)
				e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
		}
		else
			e = oe;
	}
	return 0;
}
static int yawedge(lua_State* L)
{
	float ang = lua_tonumber(L, 1);
	vec enorm = getedgenorm2(estack.back());
	rotedge(estack.back(), ang, enorm);

	coordstack.back().ux.rot(ang, enorm);
	coordstack.back().uz.rot(ang, enorm);
	return 0;
}
static int pitchedge(lua_State* L)
{
	float ang = lua_tonumber(L, 1);
	VECLIST& e = estack.back();
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);
	if (weightlist.size() == e.size())
	{
		VECLIST oe;
		rotedge(e, ang, ux, oe);
		for (int i = 0; i < e.size(); i++)
		{
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
		}
	}
	else
	{
		rotedge(e, ang, ux);
	}
	coordstack.back().uy.rot(ang, ux);
	coordstack.back().uz.rot(ang, ux);
	return 0;
}
static int rolledge(lua_State* L)
{
	float ang = lua_tonumber(L, 1);
	VECLIST& e = estack.back();
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);
	rotedge(e, ang, uz);
	coordstack.back().uy.rot(ang, uz);
	coordstack.back().ux.rot(ang, uz);
	return 0;
}
void wscaleedge(VECLIST& e, real sx, real sz)
{
	ASSERT(e.size() > 2)
		bool bwgt = weightlist.size() == e.size();
	vec o;
	if (bwgt)
	{
		int n = (e[0].p == e[e.size() - 1].p) ? e.size() - 1 : e.size();
		vec oo = getedgecenter(e, n);
		for (int i = 0; i < n; i++)
		{
			o = o + (e[i].p - oo) * weightlist[i];
		}
		o = oo + o / n;
	}
	else
		o = getedgecenter(e);


	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);
	for (int i = 0; i < e.size(); i++)
	{
		vertex& v = e[i];
		vec d = v.p - o;
		vec dx = ux * d.dot(ux);
		vec dz = uz * d.dot(uz);
		if (bwgt)
		{
			real sx2 = blend(1, sx, weightlist[i]);
			real sz2 = blend(1, sz, weightlist[i]);
			d = dx * sx2 + dz * sz2;
		}
		else
		{
			d = dx * sx + dz * sz;
		}
		v.p = o + d;
		v.ind = -1;
	}
}
void wscaleedge2(VECLIST& e, real s)
{
	ASSERT(e.size() > 2);
	bool bwgt = weightlist.size() == e.size();
	vec o = getedgecenter(e);

	int len = e.size();
	int sig = len / gwtimes;
	int n = len / sig;
	for (int j = 0; j < n; j++)
		for (int i = 0; i < sig / 2; i++)
		{
			vertex& v1 = e[j * sig + i];
			vertex& v2 = e[j * sig + sig - 1 - i];
			if (bwgt)
			{
				vec tp = v1.p;
				v1.p = blend(v1.p, v2.p, s * weightlist[i]);
				v2.p = blend(v2.p, tp, s * weightlist[i]);
			}
			else
			{
				vec tp = v1.p;
				v1.p = blend(v1.p, v2.p, s);
				v2.p = blend(v2.p, tp, s);
			}
			v1.ind = -1;
			v2.ind = -1;
		}
}
static int scaleedge(lua_State* L)
{
	if (lua_gettop(L) == 1)
	{
		float s = lua_tonumber(L, 1);
		wscaleedge(estack.back(), s, s);
	}
	else if (lua_gettop(L) == 2)
	{
		float sx = lua_tonumber(L, 1);
		float sz = lua_tonumber(L, 2);

		wscaleedge(estack.back(), sx, sz);
	}
	return 0;
}
static int scaleedgemirror(lua_State* L)
{
	VECLIST& e = estack.back();
	int len = e.size();

	int center = len / 2;
	/*real fcenter = lua_tonumber(L, 1);
	if (fcenter > 1)
		center = fcenter;
	else*/
	//center = len * center;
	float s1 = lua_tonumber(L, 1);
	float s2 = lua_tonumber(L, 2);
	real pw = 1;
	if (lua_gettop(L) >= 3)
		pw = lua_tonumber(L, 3);

	//PRINT("len=" << len);
	VECLIST oe = e;
	{
		vec n = getedgenorm2(e);
		vec o = len % 2 == 0 ? (e[center - 1] + e[center]) / 2 : e[center];
		for (int offset = 0; offset < len / 2; offset++)
		{
			real s = blend(s1, s2, (offset + 1) / real(len / 2), pw);
			int ind = (center - offset - 1);
			if (ind >= 0)
			{
				oe[ind] = ((e[ind].p - o) * s + o);
				//PRINT("rotedgemirror1 " << ind << " ang=" << ang);
			}
			//o = e[ind].p;
		}
		//o = len % 2 == 0 ? (e[center - 1] + e[center]) / 2 : e[center];
		for (int offset = 1; offset <= len / 2; offset++)
		{
			real s = blend(s1, s2, offset / real(len / 2), pw);
			int ind = (center + offset);
			if (ind < len)
			{
				oe[ind] = ((e[ind].p - o) * s + o);
				//PRINT("rotedgemirror2 " << ind << " ang=" << ang);
			}
			//o = e[ind].p;
		}
	}
	if (weightlist.size() == len)
	{
		for (int i = 0; i < len; i++)
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
	}
	else
		e = oe;


	return 0;
}
static int scaleeedge2(lua_State* L)
{
	if (lua_gettop(L) == 1)
	{
		float s = lua_tonumber(L, 1);
		wscaleedge2(estack.back(), s);
	}
	return 0;
}
static int newedge(lua_State* L)
{
	estack.push_back(VECLIST());
	return 0;
}
static int edge(lua_State* L)
{
	VECLIST e;
	int num = lua_gettop(L);
	for (int i = 0; i < num; i += 3)
	{
		vec3 p;
		p.x = lua_tonumber(L, i + 0);
		p.y = lua_tonumber(L, i + 1);
		p.z = lua_tonumber(L, i + 2);
		e.push_back(p);
	}
	estack.push_back(e);
	return 0;
}
static int setedge(lua_State* L)
{
	VECLIST& e = estack.back();

	vec o = getedgecenter(e);

	vec n = getedgenorm2(e);
	vec ux = (e[0].p - o).normcopy();
	vec uz = n.cross(ux);

	real dang = (PI * 2) / real(e.size() - 1);
	int num = MIN(e.size(), lua_gettop(L));
	for (int i = 0; i < num; i++)
	{
		real s = lua_tonumber(L, i + 1);

		vertex& v = e[i];
		real ang = i * dang;
		v.p = (ux * cos(ang) + uz * sin(ang)) * s + o;
	}
	e.back() = e.front();
	return 0;
}

static int weight(lua_State* L)
{
	weightlist.clear();
	if (0 == lua_gettop(L))
		return 0;
	real w = lua_tonumber(L, 1);
	VECLIST& e = estack.back();
	for (int i = 0; i < e.size(); i++)
	{
		weightlist.push_back(w);
	}
	return 0;
}
static int weightblend(lua_State* L)
{
	real s1 = lua_tonumber(L, 1);
	real s2 = lua_tonumber(L, 2);
	real power = lua_tonumber(L, 3);

	VECLIST& e = estack.back();
	for (int i = 0; i < e.size(); i++)
	{
		real ai = i / real(e.size() - 1);
		real w = blendn(s1, s2, ai, gwtimes, power);

		if (i < weightlist.size())
			weightlist[i] *= w;
		else
			weightlist.push_back(w);
	}
	return 0;
}
static int weightdiv(lua_State* L)
{
	real dv = lua_tonumber(L, 1);
	real w1 = lua_tonumber(L, 2);
	real w2 = lua_tonumber(L, 3);
	real power = 1;
	if (lua_gettop(L) == 4)
		power = lua_tonumber(L, 4);
	VECLIST& e = estack.back();
	for (int i = 0; i < e.size(); i++)
	{
		real ai = i / real(e.size() - 1);
		real w;
		{
			if (ai < dv)
			{
				w = ((dv - ai) / dv);
				w = pow(fabs(sin(w * PI)), power);
				w = blend(w1, w2, w);
			}
			else
			{
				w = ((ai - dv) / (1 - dv));
				w = pow(fabs(sin(w * PI)), power);
				w = blend(w1, w2, w);
			}
		}
		if (i < weightlist.size())
			weightlist[i] *= w;
		else
			weightlist.push_back(w);
	}
	return 0;
}
static int weightblendx(lua_State* L)
{
	real s1 = lua_tonumber(L, 1);
	real s2 = lua_tonumber(L, 2);
	real pow = lua_tonumber(L, 3);

	VECLIST& e = estack.back();
	vec o = getedgecenter(e);
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);

	real r = (e[0] - o).len();
	for (int i = 0; i < e.size(); i++)
	{
		vec d = e[i].p - o;
		real a = fabs(d.dot(ux)) / (r);
		real w = blend(s1, s2, a, pow);
		if (i < weightlist.size())
			weightlist[i] *= w;
		else
			weightlist.push_back(w);
	}
	return 0;
}
static int weightblendz(lua_State* L)
{
	real s1 = lua_tonumber(L, 1);
	real s2 = lua_tonumber(L, 2);
	real pow = lua_tonumber(L, 3);

	VECLIST& e = estack.back();
	vec o = getedgecenter(e);
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);

	real r = (e[0] - o).len();
	for (int i = 0; i < e.size(); i++)
	{
		vec d = e[i].p - o;
		real a = fabs(d.dot(uz)) / (r);
		real w = blend(s1, s2, a, pow);
		if (i < weightlist.size())
			weightlist[i] *= w;
		else
			weightlist.push_back(w);
	}
	return 0;
}
static int weightblendmirror(lua_State* L)
{
	VECLIST& e = estack.back();
	int len = e.size();

	int center = len / 2;
	float s1 = lua_tonumber(L, 1);
	float s2 = lua_tonumber(L, 2);
	real pw = 1;
	if (lua_gettop(L) >= 3)
		pw = lua_tonumber(L, 3);

	//PRINT("len=" << len);
	VECLIST oe = e;
	{
		vec n = getedgenorm2(e);
		vec o = len % 2 == 0 ? (e[center - 1] + e[center]) / 2 : e[center];
		for (int offset = 0; offset < len / 2; offset++)
		{
			real s = blend(s1, s2, (offset + 1) / real(len / 2), pw);
			int ind = (center - offset - 1);
			if (ind >= 0)
			{
				real w = s;
				if (ind < weightlist.size())
					weightlist[ind] *= w;
				else
					weightlist.push_back(w);
			}
		}
		for (int offset = 1; offset <= len / 2; offset++)
		{
			real s = blend(s1, s2, offset / real(len / 2), pw);
			int ind = (center + offset);
			if (ind < len)
			{
				real w = s;
				if (ind < weightlist.size())
					weightlist[ind] *= w;
				else
					weightlist.push_back(w);
			}
		}
	}
	if (weightlist.size() == len)
	{
		for (int i = 0; i < len; i++)
			e[i].p = blend(e[i].p, oe[i].p, weightlist[i]);
	}
	else
		e = oe;


	return 0;
}
static int wtimes(lua_State* L)
{
	gwtimes = 1;
	if (lua_gettop(L) == 1)
		gwtimes = lua_tonumber(L, 1);

	return 0;
}
static int radedge(lua_State* L)
{
	VECLIST& e = estack.back();
	vec o = getedgecenter(e);
	vec ux, uy, uz;
	edgeax2(e, ux, uy, uz);

	real r = lua_tonumber(L, 1);

	r = r * (e[0] - o).len();

	for (int i = 0; i < e.size(); i++)
	{
		real ai = i / real(e.size() - 1);
		real ang = ai * PI * 2 - PI / 2;
		if (weightlist.size() == e.size())
			e[i].p = blend(e[i].p, o + ux * (r * cos(ang)) + uz * (r * sin(ang)), weightlist[i]);
		else
			e[i].p = o + ux * (r * cos(ang)) + uz * (r * sin(ang));
	}
	return 0;
}
static int face(lua_State* L)
{
	if (lua_gettop(L) == 2)
	{
		int s1 = lua_tonumber(L, 1);
		int s2 = lua_tonumber(L, 2);
		face(estack[estack.size() + s1], estack[estack.size() + s2]);
	}
	else
	{
		face(estack[estack.size() - 2], estack.back());
	}
	return 0;
}
static int faceo(lua_State* L)
{
	if (lua_gettop(L) == 3)
	{
		float x = lua_tonumber(L, 2);
		float y = lua_tonumber(L, 3);
		float z = lua_tonumber(L, 4);
		face(estack.back(), vec(x, y, z));
	}
	else if (lua_gettop(L) == 1)
	{
		float d = lua_tonumber(L, 2);
		face(estack.back(), getedgecenter(estack.back()) + getedgenorm2(estack.back()) * d);
	}
	else
	{
		float d = lua_tonumber(L, 2);
		face(estack.back(), getedgecenter(estack.back()));
	}
	return 0;
}
static int face12(lua_State* L)
{
	if (lua_gettop(L) == 3)
	{
		int s1 = lua_tonumber(L, 1);
		int s2 = lua_tonumber(L, 2);
		face12(estack[estack.size() + s1], estack[estack.size() + s2]);
	}
	else
	{
		face12(estack[estack.size() - 2], estack.back());
	}
	return 0;
}
static int subedge(lua_State* L)
{
	int start = lua_tonumber(L, 1);
	int sublen = lua_tonumber(L, 2);
	VECLIST e;
	subedge(estack.back(), start, sublen, e);
	estack.back() = e;
	return 0;
}
static int div(lua_State* L)
{
	int start = lua_tonumber(L, 1);
	real s2 = lua_tonumber(L, 2);
	int sublen = s2 < 1 ? s2 * estack.back().size() : s2;

	VECLIST e1, e2;
	subedge2(estack.back(), start, sublen, e1, e2);
	//PRINT("DIV:" << e1.size() << "," << e2.size());
	estack.push_back(e2);
	estack.push_back(e1);
	return 0;
}

static int linkedge(lua_State* L)
{
	int ls = lua_tonumber(L, 1);
	bool binv = 0;
	if (lua_gettop(L) == 2)
		binv = lua_tonumber(L, 2);
	linkedge(estack[estack.size() - 1], estack[estack.size() + ls], binv);
	return 0;
}

static int unionedge(lua_State* L)
{
	int s1 = lua_tonumber(L, 1);
	int s2 = lua_tonumber(L, 2);
	int start = lua_tonumber(L, 3);
	int len = lua_tonumber(L, 4);
	VECLIST e;
	unionedge(estack[estack.size() + s1], estack[estack.size() + s2], start, len, e);
	estack.back() = e;
	return 0;
}
static int insertver(lua_State* L)
{
	int ind = lua_tonumber(L, 1);
	{
		float x = lua_tonumber(L, 2);
		float y = lua_tonumber(L, 3);
		float z = lua_tonumber(L, 4);

		estack.back().insert(estack.back().begin() + ind, vec(x, y, z));
	}
	return 0;
}
static int addver(lua_State* L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float z = lua_tonumber(L, 3);

	estack.back().push_back(vec(x, y, z));
	return 0;
}
static int removever(lua_State* L)
{
	int ind = lua_tonumber(L, 1);
	if (ind < estack.back().size())
		estack.back().erase(estack.back().begin() + ind);
	return 0;
}
static int commonvert(lua_State* L)
{
	int b = lua_tonumber(L, 1);
	gcommonvertex = b;
	return 0;
}
static int searchcomvert(lua_State* L)
{
	int b = lua_tonumber(L, 1);
	gsearchcomvertex = b;
	gcommonvertex = b;
	return 0;
}
static int rendermod(lua_State* L)
{
	int a = lua_tonumber(L, 1);
	renderstate = a;
	return 0;
}
static int debug(lua_State* L)
{
	int a = lua_tonumber(L, 1);
	debugmode = a;
	return 0;
}
static int msgbox(lua_State* L)
{
	std::string str = lua_tostring(L, 1);
	MSGBOX(str.c_str());
	return 0;
}
static int getchar(lua_State* L)
{
	getchar();
	return 0;
}

static int invnorm(lua_State* L)
{
	int b = lua_tonumber(L, 1);
	binvnorm = b;
	return 0;
}

static int doublevnum(lua_State* L)
{
	int n = 0;
	if (lua_gettop(L) == 1)
		n = lua_tonumber(L, 1);

	doublevnum(estack.back(), n);

	return 0;
}
static int moveedgeex(lua_State* L)
{
	int n = lua_gettop(L);
	std::vector<real> slist;
	for (int i = 0; i < n; i++)
	{
		slist.push_back(lua_tonumber(L, 1));
	}
	moveedgeex(estack.back(), slist);

	return 0;
}
//------------------------------------------
// init LUA
//------------------------------------------
void reset()
{// RESET
	gCurPos = vec::ZERO;
	gUpperDir = vec::UY;
	estack.clear();

	dirstack.clear();
	verstack.clear();
	coordstack.clear();
	weightlist.clear();

	gwtimes = 1;
	binvnorm = 0;
	gcommonvertex = 0;
	gsearchcomvertex = 0;

	growtime = 0;
	growpos = -1;
	bclosed = true;
}
lua_State* L = 0;
void intlua()
{
	reset();

	/*-------- initialize Lua --------*/
	L = luaL_newstate();

	/* load Lua base libraries */
	luaL_openlibs(L);
	lua_register(L, "rnd", rnd);
	lua_register(L, "bnd", bnd);
	lua_register(L, "atan2", atan2);
	lua_register(L, "pow", pow);
	lua_register(L, "prt", print);

	lua_register(L, "color", colori);
	lua_register(L, "rgb", rgb);
	lua_register(L, "hsl", hsl);
	lua_register(L, "psz", pixsize);
	lua_register(L, "mod", rendermod);
	lua_register(L, "dbg", debug);
	lua_register(L, "getchar", getchar);
	lua_register(L, "msgbox", msgbox);

	lua_register(L, "param", setparam);
	lua_register(L, "setfloat", setfloatmap);
	lua_register(L, "getfloat", getfloatmap);
	lua_register(L, "setint", setintmap);
	lua_register(L, "setstring", setstringmap);
	lua_register(L, "prt", print);
	
	//------------------------------------------
	// 绘制
	//------------------------------------------
	lua_register(L, "pix", pixel);
	lua_register(L, "pixi", pixeli);
	lua_register(L, "triang", triang);
	lua_register(L, "pst", pset);
	
	//------------------------------------------
	// 生物形态学
	//------------------------------------------

	/* 系统 */

	lua_register(L, "push", pushe);
	lua_register(L, "pop", pope);

	lua_register(L, "pushc", pushcoord);
	lua_register(L, "coord", calccoord);
	lua_register(L, "uz", coorduz);
	lua_register(L, "ux", coordux);
	lua_register(L, "mor", mirror);

	lua_register(L, "popc", popcoord);

	lua_register(L, "pushd", setdir);
	lua_register(L, "popd", popdir);

	lua_register(L, "comv", commonvert);
	lua_register(L, "scomv", searchcomvert);
	lua_register(L, "inv", invnorm);


	/* 构造 */

	lua_register(L, "round", roundedge);
	lua_register(L, "arc", arcedge);
	//lua_register(L, "curve", curvedge);
	lua_register(L, "newe", newedge);
	lua_register(L, "edge", edge);

	/* 权重 */

	lua_register(L, "wgt", weight);
	lua_register(L, "wbnd", weightblend);
	lua_register(L, "wdiv", weightdiv);
	lua_register(L, "wbndx", weightblendx);
	lua_register(L, "wbndz", weightblendz);
	lua_register(L, "wbndm", weightblendmirror);
	lua_register(L, "wtim", wtimes);

	/* 变换 */

	lua_register(L, "ext", extrudeedge);
	lua_register(L, "ext2", extrudeedgeex);
	lua_register(L, "mov", moveedge);
	lua_register(L, "mov2", moveedgeex);

	lua_register(L, "scl", scaleedge);
	lua_register(L, "sclm", scaleedgemirror);
	lua_register(L, "scl2", scaleeedge2);

	lua_register(L, "rot", rotedge);
	lua_register(L, "rotm", rotedgemirror);
	lua_register(L, "rote", rotonedge);
	lua_register(L, "yaw", yawedge);
	lua_register(L, "pit", pitchedge);
	lua_register(L, "rol", rolledge);

	lua_register(L, "radi", radedge);

	/* 拓扑 */

	lua_register(L, "div", div);
	lua_register(L, "cls", closeedge);
	lua_register(L, "sub", subedge);
	lua_register(L, "link", linkedge);
	lua_register(L, "union", unionedge);
	lua_register(L, "double", doublevnum);

	lua_register(L, "addv", addver);
	lua_register(L, "insv", insertver);
	lua_register(L, "remv", removever);
	lua_register(L, "pos", setpos);

	/* 绘制 */

	lua_register(L, "face", face);
	lua_register(L, "faceo", faceo);
	
	//------------------------------------------
	// API
	//------------------------------------------
	registerapi(L);
	

	/* cleanup Lua */
	//lua_close(L);
}