/**
 * @mainpage RDA5807 Arduino Library implementation
 * @details RDA5807 Arduino Library implementation. This is an Arduino library for the RDA5807, BROADCAST RECEIVER.
 * @details It works with I2C protocol and can provide an easier interface to control the RDA5807 device.<br>
 * @details This library was built based on "RDA5807M - SINGLE-CHIP BROADCAST FM RADIO TUNER - Rev.1.1–Aug.2015"
 * @details and RDA microelectronics RDA5807FP - SINGLE-CHIP BROADCAST FM RADIO TUNER 
 * @details This library can be freely distributed using the MIT Free Software model.
 * @copyright Copyright (c) 2020 Ricardo Lima Caratti.
 * @author Ricardo LIma Caratti (pu2clr@gmail.com)
 */

#include <RDA5807.h>

/**
 * @defgroup GA02 Basic Functions
 * @section GA03 Basic
 */

/**
 * @ingroup GA03
 * @brief Set the Device GPIO pins
 * @details This method is useful to add control to the system via GPIO RDA devive pins.
 * @details For example: You can use these pins to control RDS and SEEK via interrupt. 
 * @details GPIOs are General Purpose I/O pin.  
 * @details GPIO setup
 * @details When GPIO1 (#1), gpioSetup can be: 00 = High impedance; 01 = Reserved; 10 = Low; 11 = High
 * @details When GPIO2 (#2), gpioSetup can be: 00 = High impedance; 01 = Interrupt (INT) 10 = Low; 11 = High
 * @details When GPIO3 (#3), gpioSetup can be: 00 = High impedance; 01 = Mono/Stereo indicator (ST) = Low; 11 = High
 * 
 * @param gpioPin   gpio number (1, 2 or 3)
 * @param gpioSetup See description above
 * @param mcuPip    MCU (Arduino) pin connected to the gpio
 */
void RDA5807::setGpio(uint8_t gpioPin, uint8_t gpioSetup, int mcuPin)
{

    switch (gpioPin) {
        case 1:
            this->gpio1Control = mcuPin;
            reg04->refined.GPIO1 = gpioSetup;
            break;
        case 2:
            this->gpio2Control = mcuPin;
            reg04->refined.GPIO2 = gpioSetup;
            break;
        case 3:
            this->gpio3Control = mcuPin;
            reg04->refined.GPIO3 = gpioSetup;
            break;
        default:
            gpio1Control = gpio2Control = gpio3Control = -1;
           
    }
    setRegister(REG04,reg04->raw);
}

/**
 * @ingroup GA02
 * @brief Gets all current device status and RDS information registers (From 0x0A to 0x0F)
 * @see RDA5807M - SINGLE-CHIP BROADCAST FMRADIO TUNER; pages 5, 9, 12 and 13. 
 * @see rda_reg0a, rda_reg0b, rda_reg0c, rda_reg0d, rda_reg0e, rda_reg0f
 * @see shadowStatusRegisters;
 */
void RDA5807::getStatusRegisters()
{
    word16_to_bytes aux;
    int i;

    Wire.requestFrom(this->deviceAddressFullAccess, 12); // This call starts reading from 0x0A register
    for (i = 0; i < 6; i++) {
        aux.refined.highByte = Wire.read();
        aux.refined.lowByte = Wire.read();
        shadowStatusRegisters[i] = aux.raw;
    }
    Wire.endTransmission();
}

/**
 * @ingroup GA02
 * @brief Gets the register content via direct access
 * @details this method is useful to deal with a specific register. 
 * @param uint8_t register number 
 * @return word16_to_bytes register content
 * @see  word16_to_bytes datatype in RDA5807.h
 */
word16_to_bytes RDA5807::getDirectRegister(uint8_t reg)
{

    word16_to_bytes aux;
    Wire.beginTransmission(this->deviceAddressDirectAccess);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(this->deviceAddressDirectAccess, 2);
    aux.refined.highByte = Wire.read();
    aux.refined.lowByte = Wire.read();
    Wire.endTransmission();

    return aux;
}

/**
 * @ingroup GA02
 * @brief Gets the register content of a given status register (from 0x0A to 0x0F) 
 * @details Useful when you need just a specific status register content.
 * @details This methos update the first element of the shadowStatusRegisters linked to the register
 * @return rdax_reg0a the reference to current value of the 0x0A register. 
 */
void *RDA5807::getStatus(uint8_t reg)
{
    word16_to_bytes aux;

    if ( reg < 0x0A || reg > 0x0F ) return NULL;  // Maybe not necessary.

    Wire.beginTransmission(this->deviceAddressDirectAccess);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(this->deviceAddressDirectAccess, 2); // reading 0x0A register
    delayMicroseconds(250);
    aux.refined.highByte = Wire.read();
    aux.refined.lowByte = Wire.read();
    Wire.endTransmission(true);
    shadowStatusRegisters[reg - 0x0A] = aux.raw;

    return &shadowStatusRegisters[reg - 0x0A];
}


/**
 * @ingroup GA02
 * @brief Sets a given value to a specific device register 
 *
 * @see RDA5807M - SINGLE-CHIP BROADCAST FMRADIO TUNER; pages 5, 9, 10 and 11. 
 * @see rda_reg02, rda_reg03, rda_reg04, rda_reg05, rda_reg06, rda_reg07
 *
 * @param reg    register number (valid values is between 0x02 and 0x07)   
 * @param value  the unsigned 16 bits word value (see rda_rec0x data types)   
 */
void RDA5807::setRegister(uint8_t reg, uint16_t value)
{
    word16_to_bytes aux;
    if (reg > 8) return; // Maybe not necessary.
    Wire.beginTransmission(this->deviceAddressDirectAccess);
    Wire.write(reg);
    aux.raw = value;
    Wire.write(aux.refined.highByte);
    Wire.write(aux.refined.lowByte);
    Wire.endTransmission();
    shadowRegisters[reg] = aux.raw;  // Updates the shadowRegisters element
    delayMicroseconds(3000); // Check
}

/**
 * @ingroup GA02
 * @brief Waits for Seek or Tune finish
 */
void RDA5807::waitAndFinishTune()
{
    do {
        getStatus(REG0A);
    } while (reg0a->refined.STC == 0);
}

/**
 * @ingroup GA02
 * @brief Resets the device
 * @details The RDA5807M is RESET itself When VIO is Power up. 
 * @details Also, it support soft reset by triggering the 0x02 register (rda_reg02) bit 1 from 0 to 1. 
 */
void RDA5807::softReset()
{
    reg02->refined.SOFT_RESET = 1;
    setRegister(REG02,reg02->raw);
}

/**
 * @ingroup GA02
 * @brief Powers the receiver on
 */
void RDA5807::powerUp()
{
    reg02->raw = 0;
    reg02->refined.NEW_METHOD = 0;
    reg02->refined.RDS_EN = 0;  // RDS disable
    reg02->refined.CLK_MODE = this->clockType;
    reg02->refined.RCLK_DIRECT_IN = this->oscillatorType;
    reg02->refined.MONO = 1;    // Force mono   
    reg02->refined.DMUTE = 1;   // Normal operation
    reg02->refined.DHIZ = 1;    // Normal operation
    reg02->refined.ENABLE = 1;
    reg02->refined.BASS = 1;
    reg02->refined.SEEK = 0;

    setRegister(REG02,reg02->raw);
    
    reg05->raw = 0x00;
    reg05->refined.INT_MODE = 0;
    reg05->refined.LNA_PORT_SEL = 2;
    reg05->refined.LNA_ICSEL_BIT = 0;
    reg05->refined.SEEKTH = 8;  // 0b1000
    reg05->refined.VOLUME = 0;

    setRegister(REG05, reg05->raw);
}

/**
 * @ingroup GA02
 * @brief Sets new demodulate method. It can improve the receiver sensitivity about 1dB
 * 
 * @param value  true or false
 */
void RDA5807::setNewDemodulateMethod(bool value) {
    reg02->refined.NEW_METHOD = value;
    setRegister(REG02,reg02->raw);
}

/**
 * @ingroup GA02
 * @brief Power the receiver off
 */
void RDA5807::powerDown()
{
    reg02->refined.SEEK = 0;
    reg02->refined.ENABLE = 0;
    setRegister(REG02, reg02->raw);
}


/**
 * @ingroup GA02
 * @brief Starts the device
 * @details You can select the colck type and the frequency 
 * @details oscillator type: OSCILLATOR_TYPE_CRYSTAL = passive crystal; OSCILLATOR_TYPE_REFCLK = active crystal or signal generator
 * @details Clock type: CLOCK_32K, CLOCK_12M, CLOCK_13M, CLOCK_19_2M, CLOCK_24M, CLOCK_26M and CLOCK_38_4M  
 * @param clock_type       Clock used.
 * @param oscillator_type  optional. Sets the Oscillator type used (default: passive Crystal).
 */
void RDA5807::setup(uint8_t clock_type, uint8_t oscillator_type)
{
    this->oscillatorType = oscillator_type;
    this->clockType = clock_type;

    Wire.begin();
    delay(1);
    powerUp();

}


/**
 * @defgroup GA03 FM Tune Functions
 * @section GA03 FM Tune
 */


/**
 * @ingroup GA03
 * @brief Sets Soft Blend. 
 * 
 * @param value  true or false
 */
 void RDA5807::setSoftBlendEnable(bool value) {
    reg07->refined.SOFTBLEND_EN = value;
    setRegister(REG07,reg07->raw);
 }

/**
 * @ingroup GA03
 * @brief Sets AFC true or false
 * 
 * @param value  true or false
 */
void RDA5807::setAFC(bool value) {
    reg04->refined.AFCD = value;
    setRegister(REG04,reg04->raw);
}


/**
 * @ingroup GA03
 * @brief Sets  LNA_ICSEL_BIT 
 * @details Lna working current bit: 0=1.8mA; 1=2.1mA; 2=2.5mA; 3=3.0mA (default 0). 
 * @param value  - 0=1.8mA; 1=2.1mA; 2=2.5mA; 3=3.0mA
 */
void RDA5807::setLnaIcSel(uint8_t value ) {
    reg05->refined.LNA_ICSEL_BIT = value;
    setRegister(REG05,reg05->raw);
}

/**
 * @ingroup GA03
 * @brief Sets LNA input port selection bit 
 * @details YOu can select: 0 = no input; 1 = LNAN; 2 = LNAP; 3: dual port input 
 * @param value  - 0 = no input; 1 = LNAN; 2 = LNAP; 3: dual port input 
 */
void RDA5807::setLnaPortSel(uint8_t value ) {
    reg05->refined.LNA_PORT_SEL = value;
    setRegister(REG05,reg05->raw);
}



/**
 * @ingroup GA03
 * @brief Sets the channel
 * @details This method tunes the rteceiver in a given channel. 
 * @details The channel can be calculated by using the follow formula
 * @details channel = (desired frequency - start band frequency) / space channel in use / 10.0);
 * 
 * @see setFrequency, setBand, setSpace
 * @see RDA5807M - SINGLE-CHIP BROADCAST FM RADIO TUNER - Rev.1.1–Aug.2015; pages 9 and 12.
 * 
 * @param channel
 */
void RDA5807::setChannel(uint16_t channel)
{
    reg03->refined.CHAN = channel;
    reg03->refined.TUNE = 1;
    reg03->refined.BAND = this->currentFMBand;
    reg03->refined.SPACE = this->currentFMSpace;
    reg03->refined.DIRECT_MODE = 0;
    setRegister(REG03, reg03->raw);
    waitAndFinishTune();
}

/**
 * @ingroup GA03
 * @brief Sets the frequency
 * @param frequency
 */
void RDA5807::setFrequency(uint16_t frequency)
{
    uint16_t channel = (frequency - this->startBand[currentFMBand] ) / (this->fmSpace[this->currentFMSpace] );
    setChannel(channel);
    this->currentFrequency = frequency;
}

/**
 * @ingroup GA03
 * @brief Increments the current frequency
 * @details The increment uses the band space as step. See array: uint16_t fmSpace[4] = {100/10, 200/10, 50/10, 25/10};
 */
void RDA5807::setFrequencyUp()
{
    if (this->currentFrequency < this->endBand[this->currentFMBand])
        this->currentFrequency += (this->fmSpace[currentFMSpace] );
    else
        this->currentFrequency = this->startBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

/**
 * @ingroup GA03
 * @brief Decrements the current frequency
 * @details The drecrement uses the band space as step. See array: uint16_t fmSpace[4] = {20, 10, 5, 1};
 */
void RDA5807::setFrequencyDown()
{
    if (this->currentFrequency > this->startBand[this->currentFMBand])
        this->currentFrequency -= (this->fmSpace[currentFMSpace] );
    else
        this->currentFrequency = this->endBand[this->currentFMBand];

    setFrequency(this->currentFrequency);
}

/**
 * @ingroup GA03
 * @brief Gets the current frequency.
 * @return uint16_t
 */
uint16_t RDA5807::getFrequency()
{
    return this->currentFrequency;
}

/**
 * @ingroup GA03
 * @brief Gets the current channel stored in 0x0A status register. 
 * 
 * @see setChannel, setFrequency, setBand, setSpace
 * @see RDA5807M - SINGLE-CHIP BROADCAST FM RADIO TUNER - Rev.1.1–Aug.2015; pages 9 and 12. 
 * 
 * @return uint16_t current channel value
 */
uint16_t RDA5807::getRealChannel()
{
    getStatus(REG0A);
    return reg0a->refined.READCHAN;
}

/**
 * @ingroup GA03
 * @brief Gets the current frequency bases on the current channel. 
 * @details The current channel is stored in the 0x0A register. This value is updated after a tune or seek operation.
 * @details The current frequency can be calculated by the formula below
 * 
 * | Band   | Formula |
 * | ------ | ------- | 
 * |    0   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 87.0 MHz |
 * | 1 or 2 | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 76.0 MHz |
 * |    3   | Frequency = Channel Spacing (kHz) x READCHAN[9:0]+ 65.0 MHz | 
 * 
 * @see setChannel, setFrequency, setBand, setSpace
 * @see RDA5807M - SINGLE-CHIP BROADCAST FM RADIO TUNER - Rev.1.1–Aug.2015; pages 9 and 12. 
 * @return uint16_t
 */
uint16_t RDA5807::getRealFrequency() {
    return getRealChannel() * (this->fmSpace[this->currentFMSpace]) + this->startBand[currentFMBand];
 }

/**
 * @ingroup GA03
 * @brief Seek function
 *
 * @param seek_mode  if 0, wrap at the upper or lower band limit and continue seeking; 1 = stop seeking at the upper or lower band limit   
 * @param direction  if 0, seek down; if 1, seek up.
 */
void RDA5807::seek(uint8_t seek_mode, uint8_t direction)
 {
     reg02->refined.SEEK = 1;
     reg02->refined.SKMODE = seek_mode;
     reg02->refined.SEEKUP = direction;
     setRegister(REG02,reg02->raw);
}

/**
 * @ingroup GA03
 * @brief Seek function
 * @details Seeks a station up or down.
 * @details Seek up or down a station and call a function defined by the user to show the frequency during the seek process. 
 * @details Seek begins at the current channel, and goes in the direction specified with the SEEKUP bit. Seek operation stops when a channel is qualified as valid according to the seek parameters, the entire band has been searched (SKMODE = 0), or the upper or lower band limit has been reached (SKMODE = 1).
 * @details The STC bit is set high when the seek operation completes and/or the SF/BL bit is set high if the seek operation was unable to find a channel qualified as valid according to the seek parameters. The STC and SF/BL bits must be set low by setting the SEEK bit low before the next seek or tune may begin.
 * @details The SEEK bit is set low and the STC bit is set high when the seek operation completes.
 * @details It is important to say you have to implement a show frequency function. This function have to get the frequency via getFrequency function.  
 * @details Example:
 * @code
 * 
 * SI470X rx;
 * 
 * void showFrequency() {
 *    uint16_t freq = rx.getFrequency();
 *    Serial.print(freq); 
 *    Serial.println("MHz ");
 * }
 * 
 * void loop() {
 *  .
 *  .
 *      rx.seek(SI470X_SEEK_WRAP, SI470X_SEEK_UP, showFrequency); // Seek Up
 *  .
 *  .
 * }
 * @endcode
 * @param seek_mode  Seek Mode; 0 = Wrap at the upper or lower band limit and continue seeking (default); 1 = Stop seeking at the upper or lower band limit.
 * @param direction  Seek Direction; 0 = Seek down (default); 1 = Seek up.
 * @param showFunc  function that you have to implement to show the frequency during the seeking process. Set NULL if you do not want to show the progress. 
 */
void RDA5807::seek(uint8_t seek_mode, uint8_t direction, void (*showFunc)())
{
    getStatus(REG0A);
    do
    {
        reg02->refined.SEEK = 1;
        reg02->refined.SKMODE = seek_mode;
        reg02->refined.SEEKUP = direction;
        setRegister(REG02, reg02->raw);
        this->currentFrequency = getRealFrequency(); // gets the current seek frequency
        if (showFunc != NULL)
        {
            showFunc();
        }
        delay(10);
        getStatus(REG0A);
    } while (reg0a->refined.STC == 0);
    waitAndFinishTune();
    setFrequency(getRealFrequency()); // Fixes station found.
}



/**
 * @ingroup GA03
 * @brief Sets RSSI Seek Threshold
 * @param  value
 */
void RDA5807::setSeekThreshold(uint8_t value)
{
    reg05->refined.SEEKTH = value;
    setRegister(REG05,reg05->raw);
}

/**
 * @ingroup GA03
 * @brief Sets the FM band. See table below.
 * 
 * FM band table 
 * 
 * | Value | Description                 | 
 * | ----- | --------------------------- | 
 * | 00    | 87–108 MHz (US/Europe)      |
 * | 01    | 76–91 MHz (Japan)           | 
 * | 10    | 76–108 MHz (world wide)     | 
 * | 11    | 65 –76 MHz (East Europe) or 50-65MHz (see bit 9 of gegister 0x06) |
 * 
 * @param band FM band index. See table above. 
 */
void RDA5807::setBand(uint8_t band)
{
    // reg03->refined.BAND = band;
    reg03->refined.BAND = this->currentFMBand = band; // Adjusted by anonimous developer
    setRegister(REG03,reg03->raw);
}

/**
 * @ingroup GA03
 * @brief Sets the FM channel space.
 * 
 * Channel space table
 * 
 * | Value | Description | 
 * | ----- | ----------- | 
 * | 00    | 100KHz      |
 * | 01    | 200KHz      | 
 * | 10    | 50KHz       | 
 * | 11    | 25KHz       | 
 * 
 * @param space FM channel space. See table above.
 * @todo make the space 01 (200kHz) work.
 */
void RDA5807::setSpace(uint8_t space)
{
    reg03->refined.SPACE = space;
    this->currentFMSpace = space;
    setRegister(REG03, reg03->raw);
}

/**
 * @ingroup GA03 - Frequency step
 * @brief Sets the FM Step;
 * @details Converts the step frequency (25, 50, 100 or 200 kHz) to Space. Invalid values will be converted to 0 (100 kHz) 
 * @param step  25, 50, 100 or 200 kHz
 * @todo Make the step 200kHz work well
 */
void RDA5807::setStep(uint8_t step)
{
    uint8_t space;
    switch (step) {
        case 100:
        space = 0; // b00
        break;
        case 200: 
        space = 1; // b01
        break;
        case 50:
        space = 2; // b10
        break;
        case 25:
        space = 3; // b11
        break;
        default:
        space = 0; 
    }
    this->setSpace(space);
}


/**
 * @ingroup GA03
 * @brief Gets the current Rssi
 * @details RSSI; 000000 = min; 111111 = max; RSSI scale is logarithmic.
 *
 * @return int
 */
int RDA5807::getRssi()
{
    getStatus(REG0B);
    return reg0b->refined.RSSI;
}


/**
 * @ingroup GA03
 * @brief Gets the Device identification
 * @return number
 */
uint16_t RDA5807::getDeviceId()
{
    word16_to_bytes aux;
    aux = getDirectRegister(0x0);
    reg00->raw = aux.raw;
    return reg00->raw;
}

/**
 * @ingroup GA03
 * @brief Sets De-emphasis.
 * @details 75 μs. Used in USA (default); 50 μs. Used in Europe, Australia, Japan.
 *
 * @param de  0 = 75 μs; 1 = 50 μs
 */
void RDA5807::setFmDeemphasis(uint8_t de) {
  reg04->refined.DE = de;
  setRegister(REG04,reg04->raw);
}

/** 
 * @defgroup GA04 RDS Functions
 * @section GA04 RDS/RBDS
 * @todo Need optimizing the method to get the RDS informastion - getStatusRegisters should be called just once at a cicle. 
 */

/**
 * @ingroup GA04
 * @brief Sets the RDS operation
 * @details Enable or Disable the RDS
 *
 * @param true = turns the RDS ON; false  = turns the RDS OFF
 */
void RDA5807::setRDS(bool value)
{
    reg02->refined.SEEK = 0;
    reg02->refined.RDS_EN = value;
    setRegister(REG02, reg02->raw);
}

/**
 * @ingroup GA04
 * @brief Sets the RBDS operation
 * @details Enable or Disable the RDS
 *
 * @param true = turns the RBDS ON; false  = turns the RBDS OFF
 */
void RDA5807::setRBDS(bool value)
{
    reg02->refined.SEEK = 0;
    reg02->refined.RDS_EN = 1;
    setRegister(REG02, reg02->raw);
    reg04->refined.RBDS = value;
    setRegister(REG04, reg04->raw);
}


/**
 * @ingroup GA04
 * @brief Returns true if RDS Ready
 * @details Read address 0Ah and check the bit RDSR.
 * @details When using the polling method, it is best not to poll continuously. The data will appear in intervals. 
 * @return true 
 * @return false 
 */
bool RDA5807::getRdsReady()
{
    getStatus(REG0A);

    return reg0a->refined.RDSR;
}

/**
 * @ingroup GA04
 * 
 * @brief Returns the current Text Flag A/B  
 * @return uint8_t current Text Flag A/B  
 */
uint8_t RDA5807::getRdsFlagAB(void)
{
    rds_blockb blkb;
    getStatusRegisters(); // TODO: Should be called just once and be processed by all RDS functions at a time.
    blkb.blockB = reg0d->RDSB;  
    return blkb.refined.textABFlag;
}

/**
 * @ingroup GA04
 * @brief Return the group type 
 * 
 * @return uint16_t 
 */
uint16_t RDA5807::getRdsGroupType()
{
    rds_blockb blkb;

    getStatusRegisters(); // TODO: Should be called just once and be processed by all RDS functions at a time.
    blkb.blockB = reg0d->RDSB; 
    return blkb.group0.groupType;
}

/**
 * @ingroup GA04
 * 
 * @brief Gets the version code (extracted from the Block B)
 * @returns  0=A or 1=B
 */
uint8_t RDA5807::getRdsVersionCode(void)
{
    rds_blockb blkb;
    getStatusRegisters();    // TODO: Should be called just once and be processed by all RDS functions at a time.
    blkb.blockB = reg0d->RDSB; 
    return blkb.refined.versionCode;
}

/**  
 * @ingroup GA04  
 * @brief Returns the Program Type (extracted from the Block B)
 * @see https://en.wikipedia.org/wiki/Radio_Data_System
 * @return program type (an integer betwenn 0 and 31)
 */
uint8_t RDA5807::getRdsProgramType(void)
{
    rds_blockb blkb;
    getStatusRegisters(); // TODO: Should be called just once and be processed by all RDS functions at a time.
    blkb.blockB = reg0d->RDSB; 
    return blkb.refined.programType;
}

/**
 * @ingroup GA04
 * 
 * @brief Process data received from group 2B
 * @param c  char array reference to the "group 2B" text 
 */
void RDA5807::getNext2Block(char *c)
{
    char raw[2];
    int i, j;
    word16_to_bytes blk;

    blk.raw = reg0f->RDSD; 

    raw[1] = blk.refined.lowByte;
    raw[0] = blk.refined.highByte;

    for (i = j = 0; i < 2; i++)
    {
        if (raw[i] == 0xD || raw[i] == 0xA)
        {
            c[j] = '\0';
            return;
        }
        if (raw[i] >= 32)
        {
            c[j] = raw[i];
            j++;
        }
        else
        {
            c[i] = ' ';
        }
    }
}

/**
 * @ingroup GA04
 * 
 * @brief Process data received from group 2A
 * 
 * @param c  char array reference to the "group  2A" text 
 */
void RDA5807::getNext4Block(char *c)
{
    char raw[4];
    int i, j;
    word16_to_bytes blk_c, blk_d;

    blk_c.raw = reg0e->RDSC; 
    blk_d.raw = reg0f->RDSD; 

    raw[0] = blk_c.refined.highByte;
    raw[1] = blk_c.refined.lowByte;
    raw[2] = blk_d.refined.highByte;
    raw[3] = blk_d.refined.lowByte;

    for (i = j = 0; i < 4; i++)
    {
        if (raw[i] == 0xD || raw[i] == 0xA)
        {
            c[j] = '\0';
            return;
        }
        if (raw[i] >= 32)
        {
            c[j] = raw[i];
            j++;
        }
        else
        {
            c[i] = ' ';
        }
    }
}

/**
 * @ingroup GA04
 * 
 * @brief Gets the RDS Text when the message is of the Group Type 2 version A
 * @return char*  The string (char array) with the content (Text) received from group 2A 
 */
char *RDA5807::getRdsText(void)
{
    static int rdsTextAdress2A;
    rds_blockb blkb;

    getStatusRegisters();

    blkb.blockB = reg0d->RDSB; 
    rdsTextAdress2A = blkb.group2.address;

    if (rdsTextAdress2A >= 16)
        rdsTextAdress2A = 0;

    getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
    rdsTextAdress2A += 4;
    return rds_buffer2A;
}

/**
 * @ingroup GA04
 * @todo RDS Dynamic PS or Scrolling PS support
 * @brief Gets the station name and other messages. 
 * 
 * @return char* should return a string with the station name. 
 *         However, some stations send other kind of messages
 */
char *RDA5807::getRdsText0A(void)
{
    static int rdsTextAdress0A;
    rds_blockb blkb;

    getStatusRegisters();
    blkb.blockB = reg0d->RDSB; 

    if (blkb.group0.groupType == 0)
    {
        // Process group type 0
        rdsTextAdress0A = blkb.group0.address;
        if (rdsTextAdress0A >= 0 && rdsTextAdress0A < 4)
        {
            getNext2Block(&rds_buffer0A[rdsTextAdress0A * 2]);
            rds_buffer0A[8] = '\0';
            return rds_buffer0A;
        }
    }
    return NULL;
}

/**
 * @ingroup @ingroup GA04
 * 
 * @brief Gets the Text processed for the 2A group
 * 
 * @return char* string with the Text of the group A2  
 */
char *RDA5807::getRdsText2A(void)
{
    static int rdsTextAdress2A;
    rds_blockb blkb;

    getStatusRegisters();

    blkb.blockB = reg0d->RDSB; 
    rdsTextAdress2A = blkb.group2.address;

    if (blkb.group2.groupType == 2)
    {
        // Process group 2A
        // Decode B block information
        if (rdsTextAdress2A >= 0 && rdsTextAdress2A < 16)
        {
            getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
            rds_buffer2A[63] = '\0';
            return rds_buffer2A;
        }
    }
    return NULL;
}

/**
 * @ingroup GA04
 * @brief Gets the Text processed for the 2B group
 * @return char* string with the Text of the group AB  
 */
char *RDA5807::getRdsText2B(void)
{
    static int rdsTextAdress2B;
    rds_blockb blkb;

    getStatusRegisters();
    blkb.blockB = reg0d->RDSB; 
    if (blkb.group2.groupType == 2)
    {
        // Process group 2B
        rdsTextAdress2B = blkb.group2.address;
        if (rdsTextAdress2B >= 0 && rdsTextAdress2B < 16)
        {
            getNext2Block(&rds_buffer2B[rdsTextAdress2B * 2]);
            return rds_buffer2B;
        }
    }
    return NULL;
}

/**
 * @ingroup GA04 
 * @todo Need to check. It is working on SI4735 and Si4703. Why not here?
 * @brief Gets the RDS time and date when the Group type is 4 
 * @return char* a string with hh:mm +/- offset
 */
char *RDA5807::getRdsTime()
{
    // Under Test and construction
    // Need to check the Group Type before.
    rds_date_time dt;
    word16_to_bytes blk_b, blk_c, blk_d;
    rds_blockb blkb;

    getStatusRegisters();

    blk_b.raw = blkb.blockB = reg0d->RDSB;   
    blk_c.raw = reg0e->RDSC;                 
    blk_d.raw = reg0f->RDSD;                 

    uint16_t minute;
    uint16_t hour;

    if (blkb.group0.groupType == 4)
    {
        char offset_sign;
        int offset_h;
        int offset_m;

        // uint16_t y, m, d;

        dt.raw[4] = blk_b.refined.lowByte;
        dt.raw[5] = blk_b.refined.highByte;

        dt.raw[2] = blk_c.refined.lowByte;
        dt.raw[3] = blk_c.refined.highByte;

        dt.raw[0] = blk_d.refined.lowByte;
        dt.raw[1] = blk_d.refined.highByte;

        // Unfortunately it was necessary to wotk well on the GCC compiler on 32-bit
        // platforms. See si47x_rds_date_time (typedef union) and CGG “Crosses boundary” issue/features.
        // Now it is working on Atmega328, STM32, Arduino DUE, ESP32 and more.
        minute = (dt.refined.minute2 << 2) | dt.refined.minute1;
        hour = (dt.refined.hour2 << 4) | dt.refined.hour1;

        offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
        offset_h = (dt.refined.offset * 30) / 60;
        offset_m = (dt.refined.offset * 30) - (offset_h * 60);

        sprintf(rds_time, "%02u:%02u %c%02u:%02u", hour, minute, offset_sign, offset_h, offset_m);

        return rds_time;
    }

    return NULL;
}

/**
 * @ingroup GA04 
 * @brief Get the Rds Sync 
 * @details Returns true if RDS currently synchronized.
 * @return true or false
 */
bool RDA5807::getRdsSync()
{
    getStatus(REG0A);
    return reg0a->refined.RDSS;
}

/**
 * @ingroup GA04 
 * @brief Get the current Block ID
 * @details 1= the block id of register 0cH,0dH,0eH,0fH is E
 * @details 0= the block id of register 0cH, 0dH, 0eH,0fH is A, B, C, D
 * @return  0= the block id of register 0cH, 0dH, 0eH,0fH is A, B, C, D; 1 = the block id of register 0cH,0dH,0eH,0fH is E
 */
uint8_t RDA5807::getBlockId()
{
    getStatus(REG0B);
    return reg0b->refined.ABCD_E;
}

/**
 * @ingroup GA04 
 * @brief Get the current Status of block B
 * 
 * Block Errors Level of RDS_DATA_1, and is always read as Errors Level of RDS BLOCK B (in RDS mode ) or E (in RBDS mode when ABCD_E flag is 1).
 * | value | description |
 * | ----- | ----------- |
 * |  00   | 0 errors requiring correction |
 * |  01   | 1~2 errors requiring correction |
 * |  10   | 3~5 errors requiring correction |
 * |  11   | 6+ errors or error in checkword, correction not possible |
 * 
 *  **Available only in RDS Verbose mode** 
 * 
 * @return  value See table above.
 */
uint8_t RDA5807::getErrorBlockB()
{
    getStatus(REG0B);
    return reg0b->refined.BLERB;
}

/**
 * @ingroup GA04 
 * @brief Returns true when the RDS system has valid information 
 * @details Returns true if RDS currently synchronized; the information are A, B, C and D blocks; and no errors 
 * @return  true or false
 */
bool RDA5807::hasRdsInfo() {
    getStatus(REG0B);
    return  (reg0a->refined.RDSS && reg0b->refined.ABCD_E == 0 && reg0b->refined.BLERB == 0 );
}

/**
 * @ingroup GA04 
 * @brief Sets RDS fifo mode enable
 * 
 * @param value  If true, it makes the the fifo mode enable. 
 * @return true  or false
 */
void RDA5807::setRdsFifo(bool value) {
    reg04->refined.RDS_FIFO_EN = value;
    setRegister(REG04,reg04->raw);
}

/**
 * @ingroup GA04 
 * @brief Clear RDS fifo 
 * 
 * @param value  If true, it makes the the fifo mode enable. 
 * @return true  or false
 */
void RDA5807::clearRdsFifo()
{
    reg04->refined.RDS_FIFO_CLR = 1;
    setRegister(REG04, reg04->raw);
}





/** @defgroup G05 Tools method 
 * @details A set of functions used to support other functions
*/

/**
 * @ingroup G05 Covert numbers to char array
 * @brief Converts a number to a char array 
 * @details It is useful to mitigate memory space used by functions like sprintf or othetr generic similar functions
 * @details You can use it to format frequency using decimal or tousand separator and also to convert smalm numbers.      
 * 
 * @param value  value to be converted
 * @param strValue char array that will be receive the converted value 
 * @param len final string size (in bytes) 
 * @param dot the decimal or tousand separator position
 * @param separator symbol "." or "," 
 * @param remove_leading_zeros if true removes up to two leading zeros (default is true)
 */
void RDA5807::convertToChar(uint16_t value, char *strValue, uint8_t len, uint8_t dot, uint8_t separator, bool remove_leading_zeros)
{
    char d;
    for (int i = (len - 1); i >= 0; i--)
    {
        d = value % 10;
        value = value / 10;
        strValue[i] = d + 48;
    }
    strValue[len] = '\0';
    if (dot > 0)
    {
        for (int i = len; i >= dot; i--)
        {
            strValue[i + 1] = strValue[i];
        }
        strValue[dot] = separator;
    }

    if (remove_leading_zeros) { 
        if (strValue[0] == '0')
        {
            strValue[0] = ' ';
            if (strValue[1] == '0')
                strValue[1] = ' ';
        }
    }
}


/**
 * @ingroup G05 Check the I2C buss address 
 * @brief Check the I2C bus address 
 * 
 * @param uint8_t address Array - this array will be populated with the I2C bus addresses found (minimum three elements)
 * @return 0 if no i2c device is found; -1 if error is found or n > 0, where n is the number of I2C bus address found 
 */
int RDA5807::checkI2C(uint8_t *addressArray) {
  Wire.begin();
  int error, address;
  int idx = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      addressArray[idx] = address;      
      idx++;
    }
    else if (error==4)
        return -1;
  }
  return idx;
}




/** 
 * @defgroup GA06 I2S Functions
 * @section  GA06 I2S
 * @details  When setting I2S_ENABLE (register 04) bit is high, the RDA5807FP can get the output signals SCK, WS, SD signals from GPIO3, GPIO1 and GPIO2 (I2S master)
 */


/**
 * @ingroup GA06 set I2S 
 * @brief Configure all parameters for I2S
 * @details I2S setup must be enabled 
 * @details I2S_SW_CNT can be: I2S_WS_STEP_48, I2S_WS_STEP_44_1, I2S_WS_STEP_32, I2S_WS_STEP_24, I2S_WS_STEP_22_05, I2S_WS_STEP_16, I2S_WS_STEP_12, I2S_WS_STEP_11_025 or I2S_WS_STEP_8
 * 
 * @param R_DELY If 1, R channel data delay 1T
 * @param L_DELY If 1, L channel data delay 1T
 * @param SCLK_O_EDGE If 1, invert sclk output when as master
 * @param SW_O_EDGE If 1, invert ws output when as master
 * @param I2S_SW_CNT Only valid in master mode. See table above
 * @param WS_I_EDGE  If 0, use normal ws internally; If 1, inverte ws internally
 * @param DATA_SIGNED If 0, I2S output unsigned 16-bit audio data. If 1, I2S output signed 16-bit audio data.
 * @param SCLK_I_EDGE If 0, use normal sclk internally;If 1, inverte sclk internally
 * @param WS_LR Ws relation to l/r channel; If 0, ws=0 ->r, ws=1 ->l; If 1, ws=0 ->l, ws=1 ->r
 * @param SLAVE_MASTER I2S slave or master; 1 = slave; 0 = master
 * @param OPEN_MODE Open reserved register mode;  11=open behind registers writing function others: only open behind registers reading function
 * 
 * @see RDA microelectronics RDA5807FP - SINGLE-CHIP BROADCAST FM RADIO TUNER pages 11 and 12 
 * 
 * @see setI2SOn
 */
void RDA5807::setI2SAllParameters(uint8_t R_DELY, uint8_t L_DELY, uint8_t SCLK_O_EDGE, uint8_t SW_O_EDGE, uint8_t I2S_SW_CNT, uint8_t WS_I_EDGE, uint8_t DATA_SIGNED, uint8_t SCLK_I_EDGE, uint8_t WS_LR, uint8_t SLAVE_MASTER, uint8_t OPEN_MODE ) {
    reg06->refined.R_DELY = R_DELY;
    reg06->refined.L_DELY = L_DELY;
    reg06->refined.SCLK_O_EDGE = SCLK_O_EDGE;
    reg06->refined.SW_O_EDGE = SW_O_EDGE;
    reg06->refined.I2S_SW_CNT = I2S_SW_CNT;
    reg06->refined.WS_I_EDGE = WS_I_EDGE;
    reg06->refined.DATA_SIGNED = DATA_SIGNED;
    reg06->refined.SCLK_I_EDGE = SCLK_I_EDGE;
    reg06->refined.WS_LR = WS_LR;
    reg06->refined.SLAVE_MASTER = SLAVE_MASTER; 
    reg06->refined.OPEN_MODE = OPEN_MODE;   

    setRegister(REG06,reg06->raw);
}



/**
 * @ingroup GA06 set I2S on or off
 * @brief Enable I2S setup
 * @details  When setting I2S_ENABLE (register 04) bit is high, the RDA5807FP you can get the output signals SCK, WS, SD signals from GPIO3, GPIO1 and  GPIO2 (I2S master)
 * 
 * @param value  true or false
 */
void RDA5807::setI2SOn(bool value) {
    reg04->refined.I2S_ENABLE = value;
    setRegister(REG04,reg04->raw);
}


/**
 * @ingroup GA06 set I2S Slave or Master
 * @brief 
 * 
 * @param value  true or false
 */
void RDA5807::setI2SMaster(bool value) {
    reg06->refined.SLAVE_MASTER = !value; 
    setRegister(REG06,reg06->raw);
}



/**
 * @ingroup GA06 set I2S STEP/SPEED
 * @brief Sets the speed in kbps. You can use the predefined constantes: I2S_WS_STEP_48, I2S_WS_STEP_44_1, I2S_WS_STEP_32,
 * @brief I2S_WS_STEP_24, I2S_WS_STEP_22_05, I2S_WS_STEP_16, I2S_WS_STEP_12, I2S_WS_STEP_11_025 or I2S_WS_STEP_8    
 * 
 * @param value value 
 */
void RDA5807::setI2SSpeed(uint8_t value) {
    reg06->refined.I2S_SW_CNT = value;
    setRegister(REG06,reg06->raw);
}


/**
 * @ingroup GA06 set I2S Data Signed
 * @brief If 0, I2S output unsigned 16-bit audio data. If 1, I2S output signed 16-bit audio data.
 * 
 * @param value  true (1) or false (0)
 */
void RDA5807::setI2SDataSigned(bool value) {
    reg06->refined.DATA_SIGNED = value; 
    setRegister(REG06,reg06->raw);
}




/** 
 * @defgroup GA07 Audio Functions
 * @section  GA07 Audio
 */



/**
 * @ingroup GA07
 * @brief Sets Soft Mute Enable or disable 
 * @param value true = enable; false=disable
 */
void RDA5807::setSoftmute(bool value)
{
    reg04->refined.SOFTMUTE_EN = value;
    setRegister(REG04, reg04->raw);
}




/**
 * @ingroup GA07
 * @brief Sets Audio mute or unmute
 * @param value TRUE = mute; FALSE = unmute
 */
void RDA5807::setMute(bool value)
{
    reg02->refined.SEEK = 0;    
    reg02->refined.DMUTE = !value;  // 1 = Normal operation; 0 = Mute
    setRegister(REG02,reg02->raw); 
}

/**
 * @ingroup GA07
 * @brief Sets audio output impedance high ow low
 * @param value TRUE = High; FALSE = Low 
 */
void RDA5807::setAudioOutputHighImpedance(bool value)
{
    reg02->refined.SEEK = 0;    
    reg02->refined.DHIZ = !value; // 0 = High impedance; 1 = Normal operation
    setRegister(REG02,reg02->raw); 
}


/**
 * @ingroup GA07
 * @brief Sets audio Mono or stereo
 *
 * @param value TRUE = Mono; FALSE force stereo
 */
void RDA5807::setMono(bool value)
{
    reg02->refined.SEEK = 0;
    reg02->refined.MONO = value;
    setRegister(REG02, reg02->raw);
}

/**
 * @ingroup GA07
 * @brief Sets Bass Boost
 *
 * @param value FALSE = Disable; TRUE = Enable
 */
void RDA5807::setBass(bool value)
{
    reg02->refined.SEEK = 0;
    reg02->refined.BASS = value;
    setRegister(REG02, reg02->raw);
}

/**
 * @ingroup GA07
 * @brief Gets the current Stereo status
 *
 * @return TRUE if stereo; 
 */
bool RDA5807::isStereo()
{
    getStatus(REG0A);
    return reg0a->refined.ST;
}



/**
 * @ingroup GA07
 * @brief Sets the audio volume level
 *
 * @param value
 */
void RDA5807::setVolume(uint8_t value)
{
    if ( value > 15 ) value = 15;

    reg05->refined.VOLUME = this->currentVolume = value;
    setRegister(REG05, reg05->raw);
}

/**
 * @ingroup GA07
 * @brief Gets the current audio volume level
 *
 * @return uint8_t  0 to 15
 */
uint8_t RDA5807::getVolume()
{
    return this->currentVolume;
}

/**
 * @ingroup GA07
 * @brief Increments the audio volume
 *
 */
void RDA5807::setVolumeUp()
{
    if (this->currentVolume < 15)
    {
        this->currentVolume++;
        setVolume(this->currentVolume);
    }
}

/**
 * @ingroup GA07
 * @brief Decrements the audio volume
 *
 */
void RDA5807::setVolumeDown()
{
    if (this->currentVolume > 0)
    {
        this->currentVolume--;
        setVolume(this->currentVolume);
    }
}

