/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.
 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").
 Contact information
 -------------------
 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#ifndef _ps4parser_h_
#define _ps4parser_h_

#include "Usb.h"
#include "controllerEnums.h"

/** Botones del mando */
const uint8_t PS4_BUTTONS[] PROGMEM = {
        UP, // UP
        RIGHT, // RIGHT
        DOWN, // DOWN
        LEFT, // LEFT

        0x0C, // SHARE
        0x0D, // OPTIONS
        0x0E, // L3
        0x0F, // R3

        0x0A, // L2
        0x0B, // R2
        0x08, // L1
        0x09, // R1

        0x07, // TRIANGLE
        0x06, // CIRCLE
        0x05, // CROSS
        0x04, // SQUARE

        0x10, // PS
        0x11, // TOUCHPAD
};

union PS4Buttons {
        struct {
                uint8_t dpad : 4;
                uint8_t square : 1;
                uint8_t cross : 1;
                uint8_t circle : 1;
                uint8_t triangle : 1;

                uint8_t l1 : 1;
                uint8_t r1 : 1;
                uint8_t l2 : 1;
                uint8_t r2 : 1;
                uint8_t share : 1;
                uint8_t options : 1;
                uint8_t l3 : 1;
                uint8_t r3 : 1;

                uint8_t ps : 1;
                uint8_t touchpad : 1;
                uint8_t reportCounter : 6;
        } __attribute__((packed));
        uint32_t val : 24;
} __attribute__((packed));

struct touchpadXY {
        uint8_t dummy; // No consigo averiguar para qué sirven estos datos, parece que cambian aleatoriamente, ¿quizás una marca de tiempo?
        struct {
                uint8_t counter : 7; // Aumenta cada vez que un dedo toca el panel táctil
                uint8_t touching : 1; // El bit superior se borra si el dedo está tocando el touchpad
                uint16_t x : 12;
                uint16_t y : 12;
        } __attribute__((packed)) finger[2]; // 0 = primer dedo, 1 = segundo dedo
} __attribute__((packed));

struct PS4Status {
        uint8_t battery : 4;
        uint8_t usb : 1;
        uint8_t audio : 1;
        uint8_t mic : 1;
        uint8_t unknown : 1; // ¿Puerto de extensión?
} __attribute__((packed));

struct PS4Data {
        /* Valores de botones y joystick */
        uint8_t hatValue[4];
        PS4Buttons btn;
        uint8_t trigger[2];

        /* Valores del giroscopio y el acelerómetro */
        uint8_t dummy[3]; // Los dos primeros parecen aleatorios, mientras que el tercero podría ser algún tipo de estado, ya que se incrementa de vez en cuando.
        int16_t gyroY, gyroZ, gyroX;
        int16_t accX, accZ, accY;

        uint8_t dummy2[5];
        PS4Status status;
        uint8_t dummy3[3];

        /* El resto son datos para el touchpad */
        touchpadXY xy[3]; // Parece que envía tres coordenadas cada vez, esto puede ser debido a que el microcontrolador dentro del controlador PS4 es mucho más rápido que la conexión Bluetooth.
                          // El último dato se lee de la última posición del array mientras que la medida más antigua es de la primera posición.
                          // La primera posición también mantendrá su valor después de soltar el dedo, mientras que las otras dos se pondrán a cero.
                          // Ten en cuenta que si lees lo suficientemente rápido del dispositivo, sólo la primera contendrá algún dato.

        // Los últimos tres bytes son siempre: 0x00, 0x80, 0x00
} __attribute__((packed));

struct PS4Output {
        uint8_t bigRumble, smallRumble; // Rumble
        uint8_t r, g, b; // RGB
        uint8_t flashOn, flashOff; // Tiempo de parpadeo claro/oscuro (255 = 2,5 segundos)
        bool reportChanged; // Los datos se envían cuando se reciben datos del controlador
} __attribute__((packed));

/** Esta clase analiza todos los datos enviados por el controlador PS4 */
class PS4Parser {
public:
        /** Constructor para la clase PS4Parser. */
        PS4Parser() {
                Reset();
        };

        /** @nombre Funciones del mando PS4 */
        /**
         * getButtonPress(ButtonEnum b) devolverá true mientras se mantenga pulsado el botón.
         *
         * Mientras que getButtonClick(ButtonEnum b) sólo lo devolverá una vez.
         *
         * Así, por ejemplo, si necesitas aumentar una variable una vez, utilizarías getButtonClick(ButtonEnum b),
         * pero si necesitas hacer avanzar un robot usarías getButtonPress(ButtonEnum b).
         * @param b           ::ButtonEnum para leer.
         * @return            getButtonPress(ButtonEnum b) devolverá un true mientras se mantenga pulsado un botón, mientras que getButtonClick(ButtonEnum b) devolverá true una vez por cada pulsación de botón.
         */
        bool getButtonPress(ButtonEnum b);
        bool getButtonClick(ButtonEnum b);
        /**@}*/
        /** @nombre Funciones del mando PS4 */
        /**
         * Se utiliza para obtener el valor analógico de las pulsaciones de botón.
         * @param b          El ::ButtonEnum a leer.
         * Los botones compatibles son:
         * ::L2 and ::R2.
         * @return           Valor analógico en el rango de 0-255.
         */
        uint8_t getAnalogButton(ButtonEnum b);

        /**
         * Se utiliza para leer el joystick analógico.
         * @param  a   ::LeftHatX, ::LeftHatY, ::RightHatX, and ::RightHatY.
         * @return     Devuelve el valor analógico en el rango de 0-255.
         */
        uint8_t getAnalogHat(AnalogHatEnum a);

        /**
         * Obtiene la coordenada x del touchpad. La posición 0 está en la parte superior izquierda.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   El controlador envía tres paquetes con la misma estructura.
         *                El tercero contendrá el último compás, pero si se lee desde el controlador sólo habrá datos en el primero.
         *                Por eso se pondrá a 0 si se omite el argumento.
         * @return        Devuelve la coordenada x del dedo.
         */
        uint16_t getX(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].x;
        };

        /**
         * Obtiene la coordenada y del touchpad. La posición 0 está en la parte superior izquierda.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Returns the y-coordinate of the finger.
         */
        uint16_t getY(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].y;
        };

        /**
         * Aparece cuando el usuario toca el panel táctil.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Returns true if the specific finger is touching the touchpad.
         */
        bool isTouching(uint8_t finger = 0, uint8_t xyId = 0) {
                return !(ps4Data.xy[xyId].finger[finger].touching); // The bit is cleared when a finger is touching the touchpad
        };

        /**
         * Este contador se incrementa cada vez que un dedo toca el panel táctil.
         * @param  finger 0 = first finger, 1 = second finger. If omitted, then 0 will be used.
         * @param  xyId   The controller sends out three packets with the same structure.
         *                The third one will contain the last measure, but if you read from the controller then there is only be data in the first one.
         *                For that reason it will be set to 0 if the argument is omitted.
         * @return        Return the value of the counter, note that it is only a 7-bit value.
         */
        uint8_t getTouchCounter(uint8_t finger = 0, uint8_t xyId = 0) {
                return ps4Data.xy[xyId].finger[finger].counter;
        };

        /**
         * Obtén el ángulo del controlador calculado mediante el acelerómetro.
         * @param  a Either ::Pitch or ::Roll.
         * @return   Devuelve el ángulo en el rango de 0-360.
         */
        float getAngle(AngleEnum a) {
                if (a == Pitch)
                        return (atan2f(ps4Data.accY, ps4Data.accZ) + PI) * RAD_TO_DEG;
                else
                        return (atan2f(ps4Data.accX, ps4Data.accZ) + PI) * RAD_TO_DEG;
        };

        /**
         * Se utiliza para obtener los valores brutos del giroscopio de 3 ejes y el acelerómetro de 3 ejes del mando de PS4.
         * @param  s El sensor para leer.
         * @return   Devuelve la lectura del sensor sin procesar.
         */
        int16_t getSensor(SensorEnum s) {
                switch(s) {
                        case gX:
                                return ps4Data.gyroX;
                        case gY:
                                return ps4Data.gyroY;
                        case gZ:
                                return ps4Data.gyroZ;
                        case aX:
                                return ps4Data.accX;
                        case aY:
                                return ps4Data.accY;
                        case aZ:
                                return ps4Data.accZ;
                        default:
                                return 0;
                }
        };

        /**
         * Devuelve el nivel de batería del mando PS4.
         * @return El nivel de batería en el rango 0-15.
         */
        uint8_t getBatteryLevel() {
                return ps4Data.status.battery;
        };

        /**
         * Sirve para comprobar si hay un cable USB conectado al mando de PS4.
         * @return Devuelve true si hay un cable USB conectado.
         */
        bool getUsbStatus() {
                return ps4Data.status.usb;
        };

        /**
         * Sirve para comprobar si hay un cable de audio conectado al mando de PS4.
         * @return Devuelve true si hay un cable de audio conectado.
         */
        bool getAudioStatus() {
                return ps4Data.status.audio;
        };

        /**
         * Permite comprobar si hay un micrófono conectado al mando de PS4.
         * @return Devuelve true si hay un micrófono conectado.
         */
        bool getMicStatus() {
                return ps4Data.status.mic;
        };

        /** Apaga el Rumble y los LEDs. */
        void setAllOff() {
                setRumbleOff();
                setLedOff();
        };

        /** Apaga el Rumble. */
        void setRumbleOff() {
                setRumbleOn(0, 0);
        };

        /**
         * Enciende el rumble.
         * @param mode Either ::RumbleHigh or ::RumbleLow.
         */
        void setRumbleOn(RumbleEnum mode) {
                if (mode == RumbleLow)
                        setRumbleOn(0x00, 0xFF);
                else
                        setRumbleOn(0xFF, 0x00);
        };

        /**
         * Enciende el rumble.
         * @param bigRumble   Valor para motor grande.
         * @param smallRumble Valor para motor pequeño.
         */
        void setRumbleOn(uint8_t bigRumble, uint8_t smallRumble) {
                ps4Output.bigRumble = bigRumble;
                ps4Output.smallRumble = smallRumble;
                ps4Output.reportChanged = true;
        };

        /** Apaga todos los LED. */
        void setLedOff() {
                setLed(0, 0, 0);
        };

        /**
         * Utilícelo para establecer el color utilizando valores RGB.
         * @param r,g,b RGB value.
         */
        void setLed(uint8_t r, uint8_t g, uint8_t b) {
                ps4Output.r = r;
                ps4Output.g = g;
                ps4Output.b = b;
                ps4Output.reportChanged = true;
        };

        /**
         * Utilícelo para establecer el color utilizando los colores predefinidos en ::ColorsEnum.
         * @param color El color deseado.
         */
        void setLed(ColorsEnum color) {
                setLed((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color));
        };

        /**
         * Ajusta el tiempo de parpadeo de los LEDs.
         * @param flashOn  Tiempo de brillar (255 = 2.5 seconds).
         * @param flashOff Tiempo de parpadear oscuro (255 = 2.5 seconds).
         */
        void setLedFlash(uint8_t flashOn, uint8_t flashOff) {
                ps4Output.flashOn = flashOn;
                ps4Output.flashOff = flashOff;
                ps4Output.reportChanged = true;
        };
        /**@}*/

protected:
        /**
         * Used to parse data sent from the PS4 controller.
         * @param len Length of the data.
         * @param buf Pointer to the data buffer.
         */
        void Parse(uint8_t len, uint8_t *buf);

        /** Used to reset the different buffers to their default values */
        void Reset();

        /**
         * Send the output to the PS4 controller. This is implemented in PS4BT.h and PS4USB.h.
         * @param output Pointer to PS4Output buffer;
         */
        virtual void sendOutputReport(PS4Output *output) = 0;

private:
        static int8_t getButtonIndexPS4(ButtonEnum b);
        bool checkDpad(ButtonEnum b); // Se utiliza para comprobar los botones DPAD de PS4

        PS4Data ps4Data;
        PS4Buttons oldButtonState, buttonClickState;
        PS4Output ps4Output;
        uint8_t oldDpad;
};
#endif
