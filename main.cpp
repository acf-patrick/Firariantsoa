#include <cmath>
#include <ctime>
#include <vector>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

// [a; b]
int randint(int, int);

const float pi = 3.14159265358979323846;

const int width(480), height(360);

struct Vector
{
	float x, y;
	Vector(float _x = 0, float _y = 0) : x(_x), y(_y)
	{}

	float magnitude()
	{ return std::sqrt(x*x + y*y); }

	void normalize()
	{
		float n = magnitude();
		if (n != 0.f)
		{
			x /= n;
			y /= n;
		}
	}

    Vector operator+(const Vector& v)
    { return Vector(v.x + x, v.y + y); }

    Vector operator-(const Vector& v)
    { return Vector(x - v.x, y - v.y); }

    Vector operator*(const float f)
    { return Vector(x*f, y*f); }
};

class Dash
{
protected:
	Vector start, velocity;
	float v_magnitude;
	Uint32 color;

	static const int len;

public:

    Dash(int x, int y, float a, Uint32 c) :
    	start(x, y),
    	velocity(std::cos(a), std::sin(a)),
    	v_magnitude(2), color(c)
    {}

    virtual ~Dash() {}

    void draw(SDL_Surface *screen)
    {
    	Vector end(len*velocity.x, len*velocity.y);
    	end = end + start;
        lineColor(screen, Sint16(start.x), Sint16(start.y), Sint16(end.x), Sint16(end.y), color);
    }

    virtual void update()
    {
		start = start + velocity*v_magnitude;
    }

	Vector getPosition()
	{
		return start;
	}
};

const int Dash::len(10);

class Spread
{
private:

	class dash : public Dash
	{
	public:
		dash(int x, int y, float a, Uint32 c) : Dash(x, y, a, c)
		{}
		void update() override
		{
            //v_magnitude -= 0.0001;
			Dash::update();
		}
	};

	std::vector<Dash*> m_dash;
	Vector start;
	int distance;
	bool spreading;

public:
	Spread(int x, int y) :
		start(x, y), distance(randint(50, 100)),
		spreading(true)
    {
    	int n = randint(50, 100);
    	float section = 2*pi/n;
		Uint32 color = 0;
		for (int k=0; k<6; ++k)
			color += randint(6, 15)*std::pow(16, k);
        for (int i=0; i<n; ++i)
			if (rand()%2)
				m_dash.push_back(new dash(x, y, i*section, color));
    }

    ~Spread()
    {
		for (auto& d : m_dash)
			delete d;
		m_dash.clear();
    }

	void draw(SDL_Surface* screen)
	{
		if (spreading)
			for (auto& d : m_dash)
				d->draw(screen);
	}

	bool update()
	{
		if (!spreading)
			return false;

		if (!m_dash.empty())
		{
            Vector cur = m_dash[0]->getPosition();
            if ((cur - start).magnitude() >= distance)
			{
				spreading = false;
				return false;
			}
		}

		for (auto& d : m_dash)
			d->update();

		return true;
	}
};

class System
{
private:
    std::vector<Spread*> spread;
    Dash *dash;
    Vector dest;
    class m_Dash : public Dash
    {
	public:
		m_Dash(Vector dest) :
			Dash(randint(width*0.25, width*0.75), height, 0, 0xffffff)
		{
			v_magnitude = 5;
            velocity = dest - start;
            velocity.normalize();
		}
		void update() override
		{
            v_magnitude += 0.3;
            Dash::update();
		}
    };

public:
	System() : dest(randint(width/4, 0.75*width), randint(0, height/4))
	{
        dash = new m_Dash(dest);
	}

	~System()
	{
		for (auto& s : spread)
			delete s;
		delete dash;
	}

	bool update()
    {
    	if (spread.empty())
		{
            Vector position(dash->getPosition());
            if ((dest - position).magnitude() <= 10)
			{
				spread.push_back(new Spread(dest.x, dest.y));
				for (int i=0; i<2; ++i)
					for (int j=0; j<2; ++j)
						spread.push_back(new Spread(dest.x+(i?-5:5), dest.y+(j?-5:5)));
			}
			dash->update();
		}
		else
		{
			bool result(false);
			for (auto& s : spread)
				result += s->update();
			if (!result)
				return false;
		}
		return true;
    }

    void draw(SDL_Surface* screen)
    {
    	if (spread.empty())
			dash->draw(screen);
		else
			for (auto& s : spread)
				s->draw(screen);
    }
};

class Main
{
private:
	System *s;
    SDL_Surface* screen;
    static Main* self;

	Main()
	{
		SDL_Init(SDL_INIT_VIDEO);
        screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE);
        if (!screen)
		{
			std::cerr << SDL_GetError() << std::endl;
			exit(1);
		}

		TTF_Init();
	}

	~Main()
	{
		TTF_Quit();
        SDL_Quit();
	}

	void update()
	{
		if (s)
			if (!s->update())
			{
				delete s;
				s = nullptr;
			}
	}

	void draw()
	{
		SDL_FillRect(screen, NULL, 0x1e1e1e);
		if (s)
			s->draw(screen);
		SDL_Flip(screen);
	}

public:
	static void run()
	{
		if (!self)
			self = new Main;

		bool running(true);
		SDL_Event e;
		while (running)
		{
            while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN or e.type == SDL_QUIT)
				{
					running = false;
					break;
				}
				else if (e.type == SDL_MOUSEBUTTONUP)
					self->s = new System;
			}
            self->update();
            self->draw();
            SDL_Delay(25);
		}

		delete self;
	}
};

Main* Main::self = nullptr;

int randint(int a, int b)
{
    if (a > b)
		return randint(b, a);
	return a + (rand()/(float)RAND_MAX)*(b-a);
}

int main(int argc, char* argv[])
{
	srand(time(0));
	Main::run();
    return 0;
}
