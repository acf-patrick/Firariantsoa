#include <list>
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

const Uint32 bg = 0x1e1e1e;

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

	int len;

public:

    Dash(int x, int y, float a, Uint32 c) :
    	start(x, y),
    	velocity(std::cos(a), std::sin(a)),
    	v_magnitude(2), color(c),
    	len(randint(1, 10))
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

class Spread
{
private:

	class dash : public Dash
	{
	public:
		int decc;

		dash(int x, int y, float a, Uint32 c, int d) :
			Dash(x, y, a, c), decc(d)
		{}
		void update() override
		{
			color -= decc;
            v_magnitude -= 0.001;
			Dash::update();
		}
		Uint8 transparency()
		{
			return color % (16*16);
		}
	};

	std::vector<dash*> m_dash;
	Vector start;
	int distance, decc;
	bool spreading;

public:
	Spread(int x, int y) :
		start(x, y), distance(randint(30, 75)),
		decc(randint(7, 10)), spreading(true)
    {
    	int n = randint(50, 100);
    	float section = 2*pi/n;
		Uint32 color = 0xff;
		for (int k=2; k<8; ++k)
			color += randint(6, 15)*std::pow(16, k);
        for (int i=0; i<n; ++i)
			if (rand()%2)
				m_dash.push_back(new dash(x, y, i*section, color, decc));
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
			/*
            Vector cur = m_dash[0]->getPosition();
            if ((cur - start).magnitude() >= distance)
			{
				spreading = false;
				return false;
			}
			*/
            if (m_dash[0]->transparency() == 255%decc)
				return spreading = false;
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
			Dash(randint(0, width), height, 0, 0x0)
		{
			color = 0xff;
			for (int k=2; k<8; ++k)
				color += randint(6, 15)*std::pow(16, k);
			len = 15;
			v_magnitude = 5;
            velocity = dest - start;
            velocity.normalize();
		}
		void update() override
		{
            v_magnitude += 0.6;
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

	class Text
	{
	private:
		TTF_Font *font;
		static Text* prec;

	public:
		SDL_Surface* surface;
		SDL_Rect position;

		Text(const std::string& content, int font_size, SDL_Color color, Sint16 y = -1)
		{
			if (prec and y < 0)
				y = (prec->position.y + prec->surface->h) + 20;
			font = TTF_OpenFont("font.ttf", font_size);
			if (font)
			{
				surface = TTF_RenderText_Blended(font, content.c_str(), color);
				position.x = Sint16((width-surface->w)/2);
				position.y = y;
			}
			else
				std::cerr << TTF_GetError() << std::endl;
			prec = this;
		}
		~Text()
		{
			SDL_FreeSurface(surface);
			TTF_CloseFont(font);
		}
		void draw(SDL_Surface* screen)
		{
			SDL_BlitSurface(surface, NULL, screen, &position);
		}
	};

	std::list<System*> systems;
	std::vector<Text*> texts;
    SDL_Surface* screen;
    static Main* self;
    int start_time, wait;

	Main() : start_time(SDL_GetTicks()), wait(0)
	{
		SDL_Init(SDL_INIT_VIDEO);
        screen = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE);
        if (!screen)
		{
			std::cerr << SDL_GetError() << std::endl;
			exit(1);
		}

		TTF_Init();
		srand(time(0));

		texts.push_back(new Text("ARAHABA NAHATRATRA NY TAONA E!", 25, { 255, 255, 255 }, height*0.125));
		texts.push_back(new Text("2022", 100, { 0, 0, 255 }));

		Text *dolar, *text;
		std::string t[] = { "mv 2021 2022", "echo 'fahasalamana, fahombiazana, fitiavana, ...' > 2022" };
		for (int i=0; i<2; ++i)
		{
			dolar = new Text("-$", 15, { 0, 255, 72 });
			text = new Text(t[i], 15, { 186, 186, 186 });
			dolar->position.x = 20;
			text->position.x = 20 + dolar->position.x + dolar->surface->w;
			text->position.y = dolar->position.y;
			texts.push_back(dolar);
			texts.push_back(text);
		}
	}

	~Main()
	{
		for (auto& text : texts)
			delete text;
		texts.clear();
		for (auto& sys : systems)
			delete sys;
		systems.clear();
		TTF_Quit();
        SDL_Quit();
	}

	void update()
	{
        if ((SDL_GetTicks() - start_time) >= 300)
		{
            systems.push_back(new System);
            start_time = SDL_GetTicks();
		}

        if (systems.empty())
			return;

		using iter = std::list<System*>::iterator;
		std::vector<iter> to_remove;
		for (iter it=begin(systems); it != end(systems); ++it)
			if (!(*it)->update())
				to_remove.emplace_back(it);
		for (auto& it : to_remove)
		{
			delete *it;
			systems.erase(it);
		}
	}

	void draw()
	{
		SDL_FillRect(screen, NULL, bg);
        if (!systems.empty())
			for (auto& sys : systems)
				sys->draw(screen);
		for (auto& text : texts)
			text->draw(screen);
		SDL_Flip(screen);
	}

public:
	static void run()
	{
		if (!self)
			self = new Main;

		SDL_Event e;
		while (true)
		{
            SDL_PollEvent(&e);
			if (e.type == SDL_KEYDOWN or e.type == SDL_QUIT)
				break;
            self->update();
            self->draw();
            SDL_Delay(30);
		}

		delete self;
	}
};

Main::Text* Main::Text::prec = nullptr;
Main* Main::self = nullptr;

int randint(int a, int b)
{
    if (a > b)
		return randint(b, a);
	return a + (rand()/(float)RAND_MAX)*(b-a);
}

int main(int argc, char* argv[])
{
	Main::run();
    return 0;
}
