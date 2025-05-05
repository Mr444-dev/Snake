#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <cmath>
#include  <algorithm>


const int GRID_WIDTH = 25;
const int GRID_HEIGHT = 20;
const float BLOCK_SIZE = 28.f;
const float WINDOW_WIDTH = GRID_WIDTH * BLOCK_SIZE;
const float WINDOW_HEIGHT = GRID_HEIGHT * BLOCK_SIZE;
const float INITIAL_GAME_SPEED = 0.15f;
const float SPEED_INCREMENT = 0.005f;
const float MAX_SPEED = 0.05f;
const float SPIKE_TIMER = 8.0f;
const float SPIKE_ADVANCE_INTERVAL = 2.0f;


struct Point {
    int x;
    int y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};


struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Color color;
    float lifetime;
    float initialLifetime;
};

enum class Direction { UP, DOWN, LEFT, RIGHT, NONE };

enum class GameState { STARTING, PLAYING, DYING, GAME_OVER };


sf::RenderWindow window;
std::deque<Point> snake;
Point food;
Direction currentDirection = Direction::NONE;
Direction nextDirection = Direction::NONE;
sf::Clock gameClock;
float timeSinceLastUpdate = 0.f;
float currentGameSpeed = INITIAL_GAME_SPEED;
GameState currentGameState = GameState::STARTING;
int score = 0;


std::vector<Particle> deathParticles;
float dyingTimer = 0.f; const float DYING_DURATION = 0.8f;
sf::View defaultView; sf::View shakeView;
float shakeTimer = 0.f; float shakeMagnitude = 0.f; const float SHAKE_DURATION = 0.3f; const float SHAKE_INTENSITY = 4.0f;
float scorePulseTimer = 0.f; const float SCORE_PULSE_DURATION = 0.3f;
float gameOverAppearTimer = 0.f; const float GAME_OVER_APPEAR_DURATION = 0.4f;


float foodTimer = 0.f;
float spikeAdvanceTimer = 0.f;
int leftSpikeWall = 0;
int rightSpikeWall = GRID_WIDTH;
int topSpikeWall = 0;
int bottomSpikeWall = GRID_HEIGHT;


sf::Font font;
sf::Text scoreText; sf::Text instructionsText; sf::Text gameOverText; sf::Text restartText;
sf::VertexArray gridLines(sf::Lines);
sf::CircleShape foodShape(BLOCK_SIZE / 2.f);
sf::RectangleShape segmentShape(sf::Vector2f(BLOCK_SIZE * 0.9f, BLOCK_SIZE * 0.9f));
sf::CircleShape particleShape(2.f);

sf::RectangleShape spikeShape(sf::Vector2f(BLOCK_SIZE * 0.8f, BLOCK_SIZE * 0.8f));



float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void resetSpikeWalls() {
    foodTimer = 0.f;
    spikeAdvanceTimer = 0.f;
    leftSpikeWall = 0;
    rightSpikeWall = GRID_WIDTH;
    topSpikeWall = 0;
    bottomSpikeWall = GRID_HEIGHT;
}

void spawnFood() {
    bool onSnake;
    do {
        onSnake = false;
        food = {rand() % GRID_WIDTH, rand() % GRID_HEIGHT};
        for (const auto& segment : snake) {
            if (segment == food) {
                onSnake = true;
                break;
            }
        }
    } while (onSnake);
    foodShape.setPosition(food.x * BLOCK_SIZE + BLOCK_SIZE * 0.5f, food.y * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
    resetSpikeWalls();
}


void triggerDeathAnimation() {
    currentGameState = GameState::DYING;
    dyingTimer = DYING_DURATION;
    deathParticles.clear();
    sf::Color headColor = sf::Color(0, 255, 0);
    sf::Color bodyColor = sf::Color(0, 200, 0);

    for(size_t i = 0; i < snake.size(); ++i) {
        Point seg = snake[i];
        sf::Vector2f centerPos(seg.x * BLOCK_SIZE + BLOCK_SIZE * 0.5f, seg.y * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
        int numParticles = (i == 0) ? 25 : 15; // Więcej cząstek dla głowy
        for(int j = 0; j < numParticles; ++j) {
            float angle = randomFloat(0.f, 2.f * M_PI);
            float speed = randomFloat(50.f, 150.f);
            float lifetime = randomFloat(DYING_DURATION * 0.5f, DYING_DURATION);
            deathParticles.push_back({
                centerPos, // Pozycja startowa
                {std::cos(angle) * speed, std::sin(angle) * speed}, // Prędkość
                (i == 0) ? headColor : bodyColor,
                lifetime,
                lifetime
            });
        }
    }
}


void triggerCameraShake() {
    shakeTimer = SHAKE_DURATION;
    shakeMagnitude = SHAKE_INTENSITY;
}

void setupGame() {
    snake.clear();
    snake.push_front({GRID_WIDTH / 2, GRID_HEIGHT / 2});
    currentDirection = Direction::NONE;
    nextDirection = Direction::NONE;
    score = 0;
    scoreText.setString("Score: 0");
    scoreText.setScale(1.f, 1.f); // Resetuj skalę wyniku
    currentGameSpeed = INITIAL_GAME_SPEED;
    spawnFood();
    timeSinceLastUpdate = 0;
    deathParticles.clear();
    shakeTimer = 0.f;
    scorePulseTimer = 0.f;
    gameOverAppearTimer = 0.f;
    gameOverText.setScale(0.f, 0.f);
    restartText.setScale(0.f, 0.f);
    gameClock.restart();
    resetSpikeWalls();
}

void setupTexts() {
    // (Bez zmian - ładowanie czcionki i ustawienie podstawowych właściwości)
     if (!font.loadFromFile("arial.ttf")) {
         if (!font.loadFromFile("resources/arial.ttf")) {
              std::cerr << "Error loading font!" << std::endl;
              exit(1);
         }
    }
    scoreText.setFont(font); scoreText.setCharacterSize(24); scoreText.setFillColor(sf::Color::White); scoreText.setPosition(10.f, 5.f);
    instructionsText.setFont(font); instructionsText.setCharacterSize(28); instructionsText.setFillColor(sf::Color::Cyan); instructionsText.setString("Use WASD or Arrow Keys to Move\n\nPress any movement key to Start!");
    sf::FloatRect textRect = instructionsText.getLocalBounds(); instructionsText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f); instructionsText.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
    gameOverText.setFont(font); gameOverText.setCharacterSize(60); gameOverText.setFillColor(sf::Color::Red); gameOverText.setString("GAME OVER!");
    textRect = gameOverText.getLocalBounds(); gameOverText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f); gameOverText.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f - 50.f);
    restartText.setFont(font); restartText.setCharacterSize(24); restartText.setFillColor(sf::Color::Yellow); restartText.setString("Press SPACE to Restart");
    textRect = restartText.getLocalBounds(); restartText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f); restartText.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f + 50.f);
}

void setupGrid() {
    // (Bez zmian - tworzenie linii siatki)
    gridLines.clear(); sf::Color gridColor(50, 50, 50);
    for (int x = 0; x <= GRID_WIDTH; ++x) { gridLines.append(sf::Vertex(sf::Vector2f(x * BLOCK_SIZE, 0.f), gridColor)); gridLines.append(sf::Vertex(sf::Vector2f(x * BLOCK_SIZE, WINDOW_HEIGHT), gridColor)); }
    for (int y = 0; y <= GRID_HEIGHT; ++y) { gridLines.append(sf::Vertex(sf::Vector2f(0.f, y * BLOCK_SIZE), gridColor)); gridLines.append(sf::Vertex(sf::Vector2f(WINDOW_WIDTH, y * BLOCK_SIZE), gridColor)); }
}


// --- Główna Funkcja Gry ---
int main() {
    srand(static_cast<unsigned>(time(0)));

    window.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Snake++ Professional");
    window.setFramerateLimit(60);

    defaultView = window.getDefaultView(); // Zapisz domyślny widok
    shakeView = defaultView;              // Inicjalizuj widok do trzęsienia

    setupTexts();
    setupGrid();
    foodShape.setFillColor(sf::Color::Red);
    foodShape.setOrigin(BLOCK_SIZE / 2.f, BLOCK_SIZE / 2.f);
    segmentShape.setOrigin(BLOCK_SIZE * 0.45f, BLOCK_SIZE * 0.45f);
    segmentShape.setOutlineThickness(1.f);
    segmentShape.setOutlineColor(sf::Color(30, 30, 30));
    particleShape.setOrigin(particleShape.getRadius(), particleShape.getRadius()); // Origin cząstki w środku

    setupGame(); // Ustaw stan początkowy
    currentGameState = GameState::STARTING; // Zacznij od ekranu startowego

    // --- Główna Pętla Gry ---
    while (window.isOpen()) {
        float dt = gameClock.restart().asSeconds(); // Delta time

        // --- Obsługa Zdarzeń (Input) ---
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::KeyPressed) {
                switch (currentGameState) {
                    case GameState::STARTING: {
                        Direction requestedDirection = Direction::NONE;
                         if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) requestedDirection = Direction::UP;
                         else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) requestedDirection = Direction::DOWN;
                         else if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) requestedDirection = Direction::LEFT;
                         else if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) requestedDirection = Direction::RIGHT;

                        if (requestedDirection != Direction::NONE) {
                             currentGameState = GameState::PLAYING;
                             currentDirection = requestedDirection;
                             nextDirection = requestedDirection;
                             timeSinceLastUpdate = currentGameSpeed; // Wymuś aktualizację w pierwszej klatce PLAYING
                        }
                        break;
                    }
                    case GameState::PLAYING: {
                        Direction requestedDirection = nextDirection;
                         if (event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) requestedDirection = Direction::UP;
                         else if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) requestedDirection = Direction::DOWN;
                         else if (event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) requestedDirection = Direction::LEFT;
                         else if (event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) requestedDirection = Direction::RIGHT;

                        if (currentDirection == Direction::UP && requestedDirection != Direction::DOWN) nextDirection = requestedDirection;
                        else if (currentDirection == Direction::DOWN && requestedDirection != Direction::UP) nextDirection = requestedDirection;
                        else if (currentDirection == Direction::LEFT && requestedDirection != Direction::RIGHT) nextDirection = requestedDirection;
                        else if (currentDirection == Direction::RIGHT && requestedDirection != Direction::LEFT) nextDirection = requestedDirection;
                        // Jeśli currentDirection jest NONE (pierwszy ruch po starcie), to już obsłużone w STARTING
                        break;
                    }
                    case GameState::GAME_OVER: {
                        if (event.key.code == sf::Keyboard::Space) {
                            setupGame(); // Zresetuj stan gry
                            currentGameState = GameState::STARTING; // <<< POPRAWKA: Wróć do STARTING
                        }
                        break;
                    }
                    case GameState::DYING: // Brak inputu podczas animacji śmierci
                        break;
                }
            }
        }

        // Aktualizacja animacji niezależnie od stanu (np. trzęsienie, pulsowanie)
        if (shakeTimer > 0) {
            shakeTimer -= dt;
            float currentMagnitude = shakeMagnitude * (shakeTimer / SHAKE_DURATION); // Zmniejszaj intensywność
            float offsetX = randomFloat(-currentMagnitude, currentMagnitude);
            float offsetY = randomFloat(-currentMagnitude, currentMagnitude);
            shakeView.setCenter(defaultView.getCenter() + sf::Vector2f(offsetX, offsetY));
             window.setView(shakeView); // Ustaw widok tylko jeśli się trzęsie
        } else {
             window.setView(defaultView); // Wróć do normalnego widoku
        }

        if (scorePulseTimer > 0) {
             scorePulseTimer -= dt;
             float pulse = sin((SCORE_PULSE_DURATION - scorePulseTimer) / SCORE_PULSE_DURATION * M_PI); // Fala sinus 0..1..0
             float scale = 1.0f + 0.3f * pulse;
             scoreText.setScale(scale, scale);
        } else {
             scoreText.setScale(1.f, 1.f); // Wróć do normalnej skali
        }


        switch (currentGameState) {
            case GameState::PLAYING: {
                timeSinceLastUpdate += dt;
                // *** Spike Timer Logic ***
                foodTimer += dt;
                if (foodTimer >= SPIKE_TIMER) { // Start advancing spikes if food isn't eaten
                    spikeAdvanceTimer += dt;
                    if (spikeAdvanceTimer >= SPIKE_ADVANCE_INTERVAL) {
                        spikeAdvanceTimer -= SPIKE_ADVANCE_INTERVAL; // Reset timer for next interval

                        // Advance walls, ensuring they don't cross
                        if (leftSpikeWall < rightSpikeWall - 1) leftSpikeWall++;
                        if (rightSpikeWall > leftSpikeWall + 1) rightSpikeWall--; // Use rightSpikeWall > leftSpikeWall + 1 to prevent overlap
                        if (topSpikeWall < bottomSpikeWall - 1) topSpikeWall++;
                        if (bottomSpikeWall > topSpikeWall + 1) bottomSpikeWall--; // Use bottomSpikeWall > topSpikeWall + 1
                    }
                }

                if (timeSinceLastUpdate >= currentGameSpeed) {
                    timeSinceLastUpdate -= currentGameSpeed;

                    if (nextDirection != Direction::NONE) {
                        currentDirection = nextDirection;
                    }

                    if (currentDirection != Direction::NONE) {
                        Point newHead = snake.front();
                        switch (currentDirection) {
                            case Direction::UP:    newHead.y--; break;
                            case Direction::DOWN:  newHead.y++; break;
                            case Direction::LEFT:  newHead.x--; break;
                            case Direction::RIGHT: newHead.x++; break;
                            case Direction::NONE: break;
                        }

                        bool collision = false;
                        if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT ||newHead.x < leftSpikeWall || newHead.x >= rightSpikeWall ||
                    newHead.y < topSpikeWall || newHead.y >= bottomSpikeWall) {
                            collision = true; // Kolizja ze ścianą
                        } else {
                            for (size_t i = 0; i < snake.size(); ++i) {
                                if (snake[i] == newHead) {
                                    collision = true; // Kolizja z samym sobą
                                    break;
                                }
                            }
                        }

                        if (collision) {
                             triggerDeathAnimation(); // Rozpocznij animację śmierci zamiast od razu GAME OVER
                             triggerCameraShake();    // Rozpocznij trzęsienie ekranu
                        } else {
                            snake.push_front(newHead);
                            if (newHead == food) {
                                score++;
                                scoreText.setString("Score: " + std::to_string(score));
                                spawnFood();
                                scorePulseTimer = SCORE_PULSE_DURATION; // Wyzwalacz pulsowania wyniku
                                if (currentGameSpeed > MAX_SPEED) {
                                    currentGameSpeed -= SPEED_INCREMENT;
                                }
                            } else {
                                snake.pop_back();
                            }
                        }
                    }
                }
                break;
            } // Koniec case PLAYING

            case GameState::DYING: {
                 dyingTimer -= dt;
                 // Aktualizuj cząsteczki
                 for (auto it = deathParticles.begin(); it != deathParticles.end(); /* Brak inkrementacji */) {
                      it->lifetime -= dt;
                      if (it->lifetime <= 0) {
                           it = deathParticles.erase(it); // Usuń martwe cząstki
                      } else {
                           it->pos += it->vel * dt;
                           // Zmniejszaj alpha (przezroczystość) w miarę upływu życia
                           sf::Color color = it->color;
                           color.a = static_cast<sf::Uint8>(255 * (it->lifetime / it->initialLifetime));
                           it->color = color;
                           ++it;
                      }
                 }

                 // Przejdź do GAME_OVER po zakończeniu animacji
                 if (dyingTimer <= 0) {
                      currentGameState = GameState::GAME_OVER;
                      gameOverAppearTimer = GAME_OVER_APPEAR_DURATION; // Rozpocznij animację pojawiania się tekstu
                 }
                 break;
            }

             case GameState::GAME_OVER: {

                  if (gameOverAppearTimer > 0) {
                       gameOverAppearTimer -= dt;
                       float scale = 1.0f - (gameOverAppearTimer / GAME_OVER_APPEAR_DURATION);
                       scale = std::min(1.0f, std::max(0.0f, scale)); // Ogranicz skalę do [0, 1]
                       gameOverText.setScale(scale, scale);
                       restartText.setScale(scale, scale);
                  } else {
                       gameOverText.setScale(1.f, 1.f); // Upewnij się, że jest w pełnej skali
                       restartText.setScale(1.f, 1.f);
                  }
                  break;
             }

            case GameState::STARTING: // Brak logiki update dla STARTING
                break;
        }



        window.clear(sf::Color(20, 20, 20));
        window.draw(gridLines); // Rysuj siatkę zawsze

        switch (currentGameState) {
            case GameState::STARTING:
                window.draw(instructionsText);
                break;

            case GameState::PLAYING:

                 foodShape.setFillColor(sf::Color::Red);
                 window.draw(foodShape);

                spikeShape.setFillColor(sf::Color::Yellow); // Or any color you like
                spikeShape.setOrigin(BLOCK_SIZE * 0.4f, BLOCK_SIZE * 0.4f); // Center origin


                for (int x = leftSpikeWall; x < rightSpikeWall; ++x) {
                    if (topSpikeWall > 0) { // Draw top only if it has advanced
                        spikeShape.setPosition(x * BLOCK_SIZE + BLOCK_SIZE * 0.5f, (topSpikeWall -1) * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
                        window.draw(spikeShape);
                    }
                    if (bottomSpikeWall < GRID_HEIGHT) { // Draw bottom only if it has advanced
                        spikeShape.setPosition(x * BLOCK_SIZE + BLOCK_SIZE * 0.5f, bottomSpikeWall * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
                        window.draw(spikeShape);
                    }
                }
                // Draw Left and Right Spike Walls (avoid drawing corners twice)
                for (int y = topSpikeWall; y < bottomSpikeWall; ++y) {
                    if (leftSpikeWall > 0) { // Draw left only if it has advanced
                        spikeShape.setPosition((leftSpikeWall - 1) * BLOCK_SIZE + BLOCK_SIZE * 0.5f, y * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
                        window.draw(spikeShape);
                    }
                    if (rightSpikeWall < GRID_WIDTH) { // Draw right only if it has advanced
                        spikeShape.setPosition(rightSpikeWall * BLOCK_SIZE + BLOCK_SIZE * 0.5f, y * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
                        window.draw(spikeShape);
                    }
                }
                // *** End Draw Spikes ***

                 // Rysuj węża
                 for (size_t i = 0; i < snake.size(); ++i) {
                     segmentShape.setPosition(snake[i].x * BLOCK_SIZE + BLOCK_SIZE * 0.5f, snake[i].y * BLOCK_SIZE + BLOCK_SIZE * 0.5f);
                     segmentShape.setFillColor(i == 0 ? sf::Color(0, 255, 0) : sf::Color(0, 200, 0));
                     window.draw(segmentShape);
                 }

                 window.draw(scoreText);
                break;

            case GameState::DYING:
                 // Rysuj jedzenie (może być widoczne podczas animacji)
                  window.draw(foodShape);

                 for (const auto& p : deathParticles) {
                      particleShape.setPosition(p.pos);
                      particleShape.setFillColor(p.color);
                      window.draw(particleShape);
                 }
                 
                  window.draw(scoreText);
                 break;

            case GameState::GAME_OVER:
                 // Rysuj jedzenie (może być widoczne)
                  window.draw(foodShape);
                 // Rysuj wynik
                 window.draw(scoreText);
                 // Rysuj teksty końca gry (ze skalowaniem)
                 window.draw(gameOverText);
                 window.draw(restartText);
                break;
        }

        window.display();
    } // Koniec pętli gry

    return 0;
}