/**
 * \file
 * \brief Snake game
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This module implements a classic Nokia phone game: the Snake game.
 */

#ifndef SNAKE_H_
#define SNAKE_H_

/*! \todo Extend interface as needed */

/*!
 * \startet das Snake Game
 */
void startSnakeGame(void);

/*!
 * \Methode um abzufragen ob Spiel gestartet wurde
 */
bool snakeGameStarted(void);

static void SnakeTask(void *pvParameters);

/*!
 * \brief Driver de-initialization.
 */
void SNAKE_Deinit(void);

/*!
 * \brief Driver initialization.
 */
void SNAKE_Init(void);

#endif /* SNAKE_H_ */
