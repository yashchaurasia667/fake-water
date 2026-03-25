#include <iostream>
#include <random>
#include <ctime>
#include <math.h>

struct Wave
{
  float amp;
  float freq;
  float speed;
};
int num_waves;
Wave *waves;

void initGersner(int n_waves, float max_amp, float max_freq, float max_speed)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution dis(0.0f, 1.0f);
  num_waves = n_waves;
  waves = (Wave *)malloc(num_waves * sizeof(Wave));

  for (int i = 0; i < num_waves; i++)
  {
    waves[i].amp = max_amp * dis(gen);
    waves[i].freq = max_freq * dis(gen);
    waves[i].speed = max_speed * dis(gen);
  }

  // for (int i = 0; i < num_waves; i++)
  // {
  //   std::cout << waves[i].amp << std::endl;
  //   std::cout << waves[i].freq << std::endl;
  //   std::cout << waves[i].speed << std::endl;
  //   std::cout << std::endl;
  // }
}

float gerstnerHeight(int x, int y)
{
  float h = 0.0f;
  for (int i = 0; i < num_waves; i++)
    h += waves[i].amp * sin((waves[i].freq * (x + y)) + (waves[i].speed * (float)std::time(0)));
  return h;
}

int main()
{
  initGersner(4, 2.0f, 2.0f, 2.0f);
  for (int y = 0; y < 20; y++)
  {
    for (int x = 0; x < 20; x++)
      std::cout << gerstnerHeight(x, y) << " ";
    std::cout << std::endl;
  }

  return 0;
}
