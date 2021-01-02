#include <cmath>
#include <ctime>
#include <vector>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>

// [a; b]
int randint(int a, int b)
{
    if (a > b)
		return randint(b, a);
	return a + (rand()/RAND_MAX)*(b-a);
}

const float pi = 3.14159265358979323846;

struct Vector
{
	float x, y;
	Vector(float _x, float _y) : x(_x), y(_y)
	{}
    Vector operator+(const Vector& v)
    {
		return Vector(v.x + x, v.y + y);
    }
    Vector operator*(const float f)
    {
    	return Vector(x*f, y*f);
    }
};

class Dash
{
protected:
	Vector start, velocity;
	float v_magnitude;
	Uint32 color;

public:
	static const int len;

    Dash(int x, int y, float a, Uint32 c) :
    	start(x, y),
    	velocity(std::cos(a), std::sin(a)),
    	v_magnitude(0.5), color(c)
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
            v_magnitude -= 0.0001;
			Dash::update();
		}
		Vector getPosition()
		{
			return start;
		}
	};

	std::vector<dash*> m_dash;
	Vector start;
	int distance;

public:
	Spread(int x, int y) :
		start(x, y), distance(randint(100, 150))
    {
    	int n = randint(50, 100);
    	float section = 2*pi/n;
        for (int i=0; i<n; ++i)
		{
			Uint32 color = 0;
			for (int k=0; k<6; ++k)
				color += (rand()%16)*std::pow(16, k);
			m_dash.push_back(new dash(x, y, i*section, color));
		}
    }

    ~Spread()
    {
		for (auto& d : m_dash)
			delete d;
		m_dash.clear();
    }

	void draw(SDL_Surface* screen)
	{
		for (auto& d : m_dash)
			d->draw(screen);
	}

	bool update()
	{
		if (!m_dash.empty())
		{
            Vector cur = m_dash[0]->getPosition();
            if (std::sqrt(std::pow(cur.x-start.x, 2) + std::pow(cur.y-start.y, 2)) >= distance)
				return false;
		}

		for (auto& d : m_dash)
			d->update();

		return true;
	}
};

class Main
{
private:
	Spread *s;
    SDL_Surface* screen;
    static Main* self;

	Main()
	{
		SDL_Init(SDL_INIT_VIDEO);
        screen = SDL_SetVideoMode(480, 360, 32, SDL_HWSURFACE);
        if (!screen)
		{
			std::cerr << SDL_GetError() << std::endl;
			exit(1);
		}

		TTF_Init();
		srand(time(0));
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
					self->s = new Spread(e.button.x, e.button.y);
			}
            self->update();
            self->draw();
            SDL_Delay(5);
		}

		delete self;
	}
};

Main* Main::self = nullptr;

int main(int argc, char* argv[])
{
	Main::run();
    return 0;
}
