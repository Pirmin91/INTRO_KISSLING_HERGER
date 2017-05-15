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
void startSnakeGame(bool);

/*!
 * \getter Methode um abzufragen ob Spiel gestartet wurde
 */
bool getStateSnakeGame(void);

/*!
 * \brief Driver de-initialization.
 */
void SNAKE_Deinit(void);

/*!
 * \brief Driver initialization.
 */
void SNAKE_Init(void);

#endif /* SNAKE_H_ */
