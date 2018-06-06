#include "module-base.h"

#define UART0
#ifdef UART0
    #define UART_BASE       ( PERIPHERAL_BASE + 0xB000 )
#else
    #define UART_BASE       ( PERIPHERAL_BASE + 0xC000 )
#endif

#define IO_WRITE(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define IO_READ(addr) (*(volatile unsigned int *)(addr))

#define RFR  0x00 //Регистр принимающего FIFO. DLAB=0 [чтение]
#define TFR  0x00  //Регистр передающего FIFO. DLAB=0 [запись]
#define DLL  0x00  //Регистр значения делителя частоты (младший байт) . DLAB=1 [чтение][запись]

#define IER  0x04  //Регистр разрешения прерываний. DLAB=0 [чтение][запись]
#define DLM  0x04  //Регистр значения делителя частоты (старший байт).DLAB=1 [чтение][запись]

#define IIR  0x08  //Регистр распознавания прерываний. DLAB=0/1 [чтение]
#define FCR  0x08  //Регистр управления FIFO. DLAB=0/1 [запись]

#define LCR  0x0C  //Регистр управления линией. DLAB=0/1 [чтение][запись]

#define MCR  0x10  //Регистр управления модемом. DLAB=0/1 [чтение][запись]

#define LSR  0x14  //Регистр состояния линии. DLAB=0/1 [чтение]

#define MSR  0x18  // Регистр состояния модема. DLAB=0/1 [чтение]

#define SCR  0x1C  // Рабочий регистр. DLAB=0 [чтение][запись]
#define TST  0x1C  // Регистр тестового режима. DLAB=1 [запись]

/*Формат регистра LCR*/
#define DLAB        (1 << 7)     /*Бит переключения регистровых полей
                                0 – доступ к регистрам RFR, TFR, IER, SCR
                                1 - доступ к регистрам DLL,DLM,TST*/
#define SB          (1 << 6)    /*Set Break. Принудительная остановка передачи.
                                 1 – сигнал UARTxTXD устанавливается в 0 */
#define SP          (1 << 5)    /*Stick Parity. Фиксация бита четности
                                0 – Бит четности формируется на основе EPS и PEN.
                                1 – Бит четности фиксирован и зависит от состояния EPS и PEN.
                                Значение бита выдается при передаче и проверяется при приеме.
                                PEN=1, EPS=0, бит равен 1
                                PEN=1, EPS=1, бит равен 0*/
#define EPS         (1 << 4)    /*Even Parity Select. Выбор типа контроля по четности
                                0 – контроль нечетности
                                1 – контроль четности*/
#define PEN         (1 << 3)    /*Parity Enable. Разрешение контроля четности.
                                0 – бит четности не посылается, не проверяется
                                1 – бит четности посылается и проверяется
                                Бит четности прибавляется после поля данных, бит остановки сле-
                                дует самым последним.*/
#define STB         (1 << 2)    /*Stop Bit Length. Количество бит stop-последовательности.
                                0 – 1 бит
                                1 – 1,5 бита (последовательность данных 5 бит)
                                2 бита (последовательность данных 6 – 8 бит)*/
#define WLS_MASK       (0x3)    /*Выбор длины передаваемого/принимаемого слова
                                00 – 5 бит
                                01 – 6 бит
                                10 – 7 бит
                                11 – 8 бит*/
#define WLS_OFFS         (0)    /**/



/*Формат регистра LSR*/
#define ERRF        (1 << 7)    /*Ошибка в приемном FIFO. Бит устанавливается, когда в приемном
                                FIFO присутствуют данные, имеющие ошибку контроля четности,
                                ошибку кадрирования или ошибку определения останова. Бит
                                сбрасывается при чтении LSR, если больше нет ошибок в прием-
                                ном FIFO, за исключением той, которая вызвала установку ERRF.*/
#define TEMT        (1 << 6)    /*Сигнал пустоты передатчика.
                                TEMT устанавливается в «1», когда сдвиговый регистр передатчика
                                и передающее FIFO становятся пустыми*/
#define THRE        (1 << 5)    /*Сигнал пустоты передающего FIFO.
                                Устанавливается в 1, когда передающее FIFO становится пустым.*/
#define BI          (1 << 4)    /*Прерывание по останову.
                                Бит устанавливается, когда UARTxRXD удерживается в “0” доль-
                                ше, чем время необходимое для передачи (START бит + DATA би-
                                ты +PARITY бит + STOP бит). BI обнуляется, когда этот регистр
                                считывается процессором.*/
#define FE          (1 << 3)    /*Ошибка кадрирования.
                                Бит устанавливается, когда принятые данные не имеют правильно-
                                го бита остановки. FE обнуляется, когда этот регистр считывается
                                процессорным ядром.*/
#define PE          (1 << 2)    /*Ошибка контроля четности.
                                Бит устанавливается, когда принятые данные не имеют правильно-
                                го бита четности. PE обнуляется, когда этот регистр считывается
                                процессорным ядром.*/
#define OE          (1 << 1)    /*Ошибка переполнения принимающего FIFO.
                                Бит устанавливается, когда принимающее FIFO полное и имеет
                                место прием следующих данных. OE обнуляется, когда этот ре-
                                гистр считывается процессорным ядром.*/
#define DR               (1)    /*Готовность данных (данные находятся в приемном FIFO).
                                Бит показывает, что в FIFO хранится более чем 1 байт данных. Бит
                                устанавливается, когда данные присутствуют в FIFO. DR обнуляет-
                                ся, когда из FIFO считаны все данные*/

/* Формат регистра FCR */

#define RCVR_MASK   (0x3 << 6) /* Пороговый уровень принимающего FIFO:
                                00 – 1 байт
                                01 – 4 байт
                                10 – 8 байт
                                11 – 14 байт*/
#define RCVR_OFFS   (6)
#define DMA_MODE    (1 << 3) /*Режим работы ПДП интерфейса:
                                0 – одиночный режим передачи
                                1 – пакетный режим передачи (этот режим не совместим с DMAC)
                                Для правильной работы контроллером ПДП, этот бит должен быть
                                обязательно сброшен в 0. */
#define TxF_RST     (1 << 2) /*Сброс передающего FIFO.  1 – сброс */
#define RxF_RST     (1 << 1) /* Сброс принимающего FIFO. 1 – сброс*/



extern void module_UART_init ();
extern void module_UART_send (char data);



