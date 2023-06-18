#ifndef TEST_TELEMETRY_PIPELINE_H
#define TEST_TELEMETRY_PIPELINE_H

#include <M5StickCPlus.h>

#include "TelemetryPipeline.h"

char pullBuffer[256];

class Test_TelemetryPipeline
{
  private:
    TelemetryPipeline m_TelPipe;
    
  public:
    Test_TelemetryPipeline()//TelemetryPipeline& pipeline) : m_TelPipe(pipeline)
    {
    }

    void pushPayload(const char* data)
    {
      Serial.printf("PushPayload %s\n",data);
      uint16_t blockMaxPayload = 0;
      bool tailDropped = false;

      uint16_t dataLength = strlen(data);
      // 1. Get the head block and populate the buffer with binary payload and set PayloadSize.
      Serial.println("0");
      BlockHeader blHead = m_TelPipe.getHeadBlockForPopulating(tailDropped);

      Serial.println("1");

      uint8_t* buffer = blHead.getBuffer(blockMaxPayload);

      Serial.println("2");

      if (dataLength <= blockMaxPayload)
      {
        blHead.setPayloadSize(dataLength);
      Serial.println("2.1");
        memcpy(buffer,data,dataLength);
      Serial.println("2.2");
        // 2. Commit the headblock, meaning that the head pointer is moved to the next block.
        bool isPipelineFull=false;
       Serial.println("2.3");
       m_TelPipe.commitPopulatedHeadBlock(blHead, isPipelineFull);
       Serial.println("2.4");
      }
      else
      {
        Serial.printf("Payload too big: %hu > %hu\n",dataLength,blockMaxPayload);
      }

      uint32_t maxBuffer=0;
      Serial.printf("t=%hu, h=%hu %s",m_TelPipe.getTailBlockIndex(),m_TelPipe.getHeadBlockIndex(),(char*)(m_TelPipe.getEntireBuffer(maxBuffer)));
      Serial.println("2.5");
      Serial.println();
    }

    void pullPayload()
    {
      // 3. Get the Tail Block for converting binary to json and sending to internet
      BlockHeader blTail;
      if (m_TelPipe.pullTailBlock(blTail))
      {
        uint16_t temp16;
        memcpy(pullBuffer,blTail.getBuffer(temp16),blTail.getPayloadSize());
        *(pullBuffer+blTail.getPayloadSize()) = NULL;
    
        // 4. Tail block successfully sent to internet, must call commit to advance tail pointer.
        m_TelPipe.tailBlockCommitted();
    
        Serial.printf("PullPayload %s\n",pullBuffer);
      }
      else
      {
        Serial.printf("PullPayload: none\n");        
      }

      uint32_t maxBuffer=0;
      Serial.printf("t=%hu, h=%hu %s",m_TelPipe.getTailBlockIndex(),m_TelPipe.getHeadBlockIndex(),(char*)(m_TelPipe.getEntireBuffer(maxBuffer)));
      Serial.println();
    }
      
    void test1()
    {    
      Serial.println("\nTest 1: Test with 4 blocks of 5 bytes each, total buffer is 20 bytes, use 3 byte payloads\n");

      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 20; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc");
      pushPayload("def");
      pushPayload("ghi");

      pullPayload();

      pushPayload("jkl");
      pushPayload("mno");

      pullPayload();
      pushPayload("pqr");

      pushPayload("stu");

      pullPayload();
      pullPayload();
      pullPayload();

      pushPayload("stu");
      pushPayload("vwx");
      pushPayload("yzA");
      pushPayload("BCD");
      pushPayload("EFG");

/* EXPECTED OUTPUT:
18:06:59.607 -> M5StickCPlus initializing...OK
18:07:00.174 -> E (641) ledc: freq_hz=0 duty_resolution=13
18:07:00.174 -> E (641) ledc: ledc_get_duty(739): LEDC is not initialized
18:07:00.174 -> 
18:07:00.174 -> Test 1: Test with 4 blocks of 5 bytes each, total buffer is 20 bytes, use 3 byte payloads
18:07:00.174 -> 
18:07:00.174 -> PushPayload abc
18:07:00.174 -> t=0, h=1 abc"""""""""""""""""
18:07:00.174 -> PushPayload def
18:07:00.174 -> t=0, h=2 abc""def""""""""""""
18:07:00.221 -> PushPayload ghi
18:07:00.221 -> t=0, h=3 abc""def""ghi"""""""
18:07:00.221 -> PullPayload abc
18:07:00.221 -> t=1, h=3 """""def""ghi"""""""
18:07:00.221 -> PushPayload jkl
18:07:00.221 -> t=1, h=0 """""def""ghi""jkl""
18:07:00.221 -> PushPayload mno
18:07:00.221 -> t=2, h=1 mno"""""""ghi""jkl""
18:07:00.221 -> PullPayload ghi
18:07:00.221 -> t=3, h=1 mno""""""""""""jkl""
18:07:00.221 -> PushPayload pqr
18:07:00.221 -> t=3, h=2 mno""pqr"""""""jkl""
18:07:00.221 -> PushPayload stu
18:07:00.221 -> t=0, h=3 mno""pqr""stu"""""""
18:07:00.221 -> PullPayload mno
18:07:00.221 -> t=1, h=3 """""pqr""stu"""""""
18:07:00.221 -> PullPayload pqr
18:07:00.221 -> t=2, h=3 """"""""""stu"""""""
18:07:00.221 -> PullPayload stu
18:07:00.221 -> t=3, h=3 """"""""""""""""""""
18:07:00.221 -> PushPayload stu
18:07:00.221 -> t=3, h=0 """""""""""""""stu""
18:07:00.221 -> PushPayload vwx
18:07:00.269 -> t=3, h=1 vwx""""""""""""stu""
18:07:00.269 -> PushPayload yzA
18:07:00.269 -> t=3, h=2 vwx""yzA"""""""stu""
18:07:00.269 -> PushPayload BCD
18:07:00.269 -> t=0, h=3 vwx""yzA""BCD"""""""
18:07:00.269 -> PushPayload EFG
18:07:00.269 -> t=1, h=0 """""yzA""BCD""EFG""
*/
    }

   void test2()
    {    
      Serial.println("\nTest 2: Test with 4 blocks of 5 bytes each, total buffer is 20 bytes, use 5 byte payloads\n");
      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 20; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc12");
      pushPayload("def34");
      pushPayload("ghi56");

      pullPayload();

      pushPayload("jkl78");
      pushPayload("mno90");

      pullPayload();
      pushPayload("pqr12");

      pushPayload("stu34");

      pullPayload();
      pullPayload();
      pullPayload();

      pushPayload("stu56");
      pushPayload("vwx78");
      pushPayload("yzA90");
      pushPayload("BCD12");
      pushPayload("EFG13");


      /* Expected output:
18:07:00.174 -> 
18:07:00.174 -> Test 2: Test with 4 blocks of 5 bytes each, total buffer is 20 bytes, use 5 byte payloads
18:07:00.174 -> 
13:41:38.535 -> PushPayload abc12
13:41:38.535 -> t=2, h=1 abc12"""""""""""""""
13:41:38.535 -> PushPayload def34
13:41:38.535 -> t=3, h=2 abc12def34""""""""""
13:41:38.535 -> PushPayload ghi56
13:41:38.535 -> t=0, h=3 abc12def34ghi56"""""
13:41:38.535 -> PullPayload abc12
13:41:38.535 -> t=1, h=3 """""def34ghi56"""""
13:41:38.535 -> PushPayload jkl78
13:41:38.535 -> t=1, h=0 """""def34ghi56jkl78
13:41:38.535 -> PushPayload mno90
13:41:38.535 -> t=2, h=1 mno90"""""ghi56jkl78
13:41:38.535 -> PullPayload ghi56
13:41:38.535 -> t=3, h=1 mno90""""""""""jkl78
13:41:38.535 -> PushPayload pqr12
13:41:38.535 -> t=3, h=2 mno90pqr12"""""jkl78
13:41:38.535 -> PushPayload stu34
13:41:38.535 -> t=0, h=3 mno90pqr12stu34"""""
13:41:38.535 -> PullPayload mno90
13:41:38.535 -> t=1, h=3 """""pqr12stu34"""""
13:41:38.535 -> PullPayload pqr12
13:41:38.535 -> t=2, h=3 """"""""""stu34"""""
13:41:38.535 -> PullPayload stu34
13:41:38.582 -> t=3, h=3 """"""""""""""""""""
13:41:38.582 -> PushPayload stu56
13:41:38.582 -> t=3, h=0 """""""""""""""stu56
13:41:38.582 -> PushPayload vwx78
13:41:38.582 -> t=3, h=1 vwx78""""""""""stu56
13:41:38.582 -> PushPayload yzA90
13:41:38.582 -> t=3, h=2 vwx78yzA90"""""stu56
13:41:38.582 -> PushPayload BCD12
13:41:38.582 -> t=0, h=3 vwx78yzA90BCD12"""""
13:41:38.582 -> PushPayload EFG13
13:41:38.582 -> t=1, h=0 """""yzA90BCD12EFG13

       */
    }

    void test3()
    {    
      Serial.println("\nTest 3: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads\n");

      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 23; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc12");
      pushPayload("def34");
      pushPayload("ghi56");

      pullPayload();

      pushPayload("jkl78");
      pushPayload("mno90");

      pullPayload();
      pushPayload("pqr12");

      pushPayload("stu34");

      pullPayload();
      pullPayload();
      pullPayload();

      pushPayload("stu56");
      pushPayload("vwx78");
      pushPayload("yzA90");
      pushPayload("BCD12");
      pushPayload("EFG13");

      /* Expected output: The $$$ chars indicate the end of the buffer that isn't used as the 5 byte payload won't fit in to the
       *  remaining space

14:07:04.132 -> Test 3: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads
14:07:04.179 -> 
14:07:04.179 -> PushPayload abc12
14:07:04.179 -> t=0, h=1 abc12"""""""""""""""$$$
14:07:04.179 -> PushPayload def34
14:07:04.179 -> t=0, h=2 abc12def34""""""""""$$$
14:07:04.179 -> PushPayload ghi56
14:07:04.179 -> t=0, h=3 abc12def34ghi56"""""$$$
14:07:04.179 -> PullPayload abc12
14:07:04.179 -> t=1, h=3 """""def34ghi56"""""$$$
14:07:04.179 -> PushPayload jkl78
14:07:04.179 -> t=1, h=0 """""def34ghi56jkl78$$$
14:07:04.179 -> PushPayload mno90
14:07:04.179 -> t=2, h=1 mno90"""""ghi56jkl78$$$
14:07:04.179 -> PullPayload ghi56
14:07:04.179 -> t=3, h=1 mno90""""""""""jkl78$$$
14:07:04.179 -> PushPayload pqr12
14:07:04.179 -> t=3, h=2 mno90pqr12"""""jkl78$$$
14:07:04.179 -> PushPayload stu34
14:07:04.179 -> t=0, h=3 mno90pqr12stu34"""""$$$
14:07:04.179 -> PullPayload mno90
14:07:04.179 -> t=1, h=3 """""pqr12stu34"""""$$$
14:07:04.179 -> PullPayload pqr12
14:07:04.227 -> t=2, h=3 """"""""""stu34"""""$$$
14:07:04.227 -> PullPayload stu34
14:07:04.227 -> t=3, h=3 """"""""""""""""""""$$$
14:07:04.227 -> PushPayload stu56
14:07:04.227 -> t=3, h=0 """""""""""""""stu56$$$
14:07:04.227 -> PushPayload vwx78
14:07:04.227 -> t=3, h=1 vwx78""""""""""stu56$$$
14:07:04.227 -> PushPayload yzA90
14:07:04.227 -> t=3, h=2 vwx78yzA90"""""stu56$$$
14:07:04.227 -> PushPayload BCD12
14:07:04.227 -> t=0, h=3 vwx78yzA90BCD12"""""$$$
14:07:04.227 -> PushPayload EFG13
14:07:04.227 -> t=1, h=0 """""yzA90BCD12EFG13$$$

       */
    }


    void test4()
    {    
      Serial.println("\nTest 4: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads\n");

      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 20; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc12");
      pushPayload("def34");
      pushPayload("ghi56");
      pushPayload("jkl78");
      pushPayload("vwxyz");

      pullPayload();

      pushPayload("mno90");

/* Expected output:

14:07:04.227 -> Test 4: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads and a 3 byte payload
14:07:04.227 -> 
14:07:04.227 -> PushPayload abc12
14:07:04.227 -> t=0, h=1 abc12"""""""""""""""
14:07:04.227 -> PushPayload def34
14:07:04.227 -> t=0, h=2 abc12def34""""""""""
14:07:04.276 -> PushPayload ghi56
14:07:04.276 -> t=0, h=3 abc12def34ghi56"""""
14:07:04.276 -> PushPayload jkl78
14:07:04.276 -> t=1, h=0 """""def34ghi56jkl78
14:07:04.276 -> PushPayload vwxyz
14:07:04.276 -> t=2, h=1 vwxyz"""""ghi56jkl78
14:07:04.276 -> PullPayload ghi56
14:07:04.276 -> t=3, h=1 vwxyz""""""""""jkl78
14:07:04.276 -> PushPayload mno90
14:07:04.276 -> t=3, h=2 vwxyzmno90"""""jkl78

 */
    }
    
    void test5()
    {    
      Serial.println("\nTest 5: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads and a 3 byte payload\n");

      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 23; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc12");
      pushPayload("def34");
      pushPayload("ghi56");
      pushPayload("jkl78");
      pushPayload("xyz");

      pullPayload();

      pushPayload("mno90");

      pushPayload("pqr12");

      pullPayload();
      pushPayload("stu34");

      pushPayload("stu34");
      pushPayload("vwx45");

      pullPayload();
      pullPayload();
      pullPayload();

      pushPayload("stu56");
      pushPayload("vwx78");
      pushPayload("yzA90");
      pushPayload("BCD12");
      pushPayload("EFG13");

      pullPayload();
      pullPayload();
      pullPayload();
      pullPayload();

      /* Expected output:

14:28:59.719 -> Test 5: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, use 5 byte payloads and a 3 byte payload
14:28:59.765 -> 
14:28:59.765 -> PushPayload abc12
14:28:59.765 -> t=0, h=1 abc12"""""""""""""""$$$
14:28:59.765 -> PushPayload def34
14:28:59.765 -> t=0, h=2 abc12def34""""""""""$$$
14:28:59.765 -> PushPayload ghi56
14:28:59.765 -> t=0, h=3 abc12def34ghi56"""""$$$
14:28:59.765 -> PushPayload jkl78
14:28:59.765 -> t=1, h=0 """""def34ghi56jkl78$$$
14:28:59.765 -> PushPayload xyz
14:28:59.765 -> t=2, h=1 xyz"""""""ghi56jkl78$$$
14:28:59.765 -> PullPayload ghi56
14:28:59.765 -> t=3, h=1 xyz""""""""""""jkl78$$$
14:28:59.765 -> PushPayload mno90
14:28:59.765 -> t=3, h=2 xyz""mno90"""""jkl78$$$
14:28:59.765 -> PushPayload pqr12
14:28:59.765 -> t=0, h=3 xyz""mno90pqr12"""""$$$
14:28:59.765 -> PullPayload xyz
14:28:59.765 -> t=1, h=3 """""mno90pqr12"""""$$$
14:28:59.765 -> PushPayload stu34
14:28:59.765 -> t=1, h=0 """""mno90pqr12stu34$$$
14:28:59.765 -> PushPayload stu34
14:28:59.765 -> t=2, h=1 stu34"""""pqr12stu34$$$
14:28:59.811 -> PushPayload vwx45
14:28:59.811 -> t=3, h=2 stu34vwx45"""""stu34$$$
14:28:59.811 -> PullPayload stu34
14:28:59.811 -> t=0, h=2 stu34vwx45""""""""""$$$
14:28:59.811 -> PullPayload stu34
14:28:59.811 -> t=1, h=2 """""vwx45""""""""""$$$
14:28:59.811 -> PullPayload vwx45
14:28:59.811 -> t=2, h=2 """"""""""""""""""""$$$
14:28:59.811 -> PushPayload stu56
14:28:59.811 -> t=2, h=3 """"""""""stu56"""""$$$
14:28:59.811 -> PushPayload vwx78
14:28:59.811 -> t=2, h=0 """"""""""stu56vwx78$$$
14:28:59.811 -> PushPayload yzA90
14:28:59.811 -> t=2, h=1 yzA90"""""stu56vwx78$$$
14:28:59.811 -> PushPayload BCD12
14:28:59.811 -> t=3, h=2 yzA90BCD12"""""vwx78$$$
14:28:59.811 -> PushPayload EFG13
14:28:59.811 -> t=0, h=3 yzA90BCD12EFG13"""""$$$
14:28:59.811 -> PullPayload yzA90
14:28:59.811 -> t=1, h=3 """""BCD12EFG13"""""$$$
14:28:59.811 -> PullPayload BCD12
14:28:59.858 -> t=2, h=3 """"""""""EFG13"""""$$$
14:28:59.858 -> PullPayload EFG13
14:28:59.858 -> t=3, h=3 """"""""""""""""""""$$$
14:28:59.858 -> PullPayload: none
14:28:59.858 -> t=3, h=3 """"""""""""""""""""$$$

       */
    }


    void test6()
    {    
      Serial.println("\nTest 6: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, push oversized payload\n");

      const uint16_t maxBufferKB = 0; const uint16_t maxBufferBytesRemainder = 23; const uint16_t maxPayload = 5;
      BlockHeader::s_overrideMaxPayloadSize(maxPayload);
      m_TelPipe.init(maxBufferKB, maxBufferBytesRemainder);

      pushPayload("abc12");
      pushPayload("def345");

      pullPayload();
      pullPayload();
      pushPayload("abc13");
      pushPayload("def14");
      pushPayload("ghi156");
      pushPayload("jkl23");
      pushPayload("mno24");
      pullPayload();
      pullPayload();
      pullPayload();
      pullPayload();
      pullPayload();

/* Expected output

14:28:59.858 -> Test 6: Test with 4 blocks of 5 bytes each, total buffer is 23 bytes, push oversized payload
14:28:59.858 -> 
14:28:59.858 -> PushPayload abc12
14:28:59.858 -> t=0, h=1 abc12"""""""""""""""$$$
14:28:59.858 -> PushPayload def345
14:28:59.858 -> Payload too big: 6 > 5
14:28:59.858 -> t=0, h=1 abc12"""""""""""""""$$$
14:28:59.858 -> PullPayload abc12
14:28:59.858 -> t=1, h=1 """"""""""""""""""""$$$
14:28:59.858 -> PullPayload: none
14:28:59.858 -> t=1, h=1 """"""""""""""""""""$$$
14:28:59.858 -> PushPayload abc13
14:28:59.858 -> t=1, h=2 """""abc13""""""""""$$$
14:28:59.858 -> PushPayload def14
14:28:59.858 -> t=1, h=3 """""abc13def14"""""$$$
14:28:59.902 -> PushPayload ghi156
14:28:59.902 -> Payload too big: 6 > 5
14:28:59.902 -> t=1, h=3 """""abc13def14"""""$$$
14:28:59.902 -> PushPayload jkl23
14:28:59.902 -> t=1, h=0 """""abc13def14jkl23$$$
14:28:59.902 -> PushPayload mno24
14:28:59.902 -> t=2, h=1 mno24"""""def14jkl23$$$
14:28:59.902 -> PullPayload def14
14:28:59.902 -> t=3, h=1 mno24""""""""""jkl23$$$
14:28:59.902 -> PullPayload jkl23
14:28:59.902 -> t=0, h=1 mno24"""""""""""""""$$$
14:28:59.902 -> PullPayload mno24
14:28:59.902 -> t=1, h=1 """"""""""""""""""""$$$
14:28:59.902 -> PullPayload: none
14:28:59.902 -> t=1, h=1 """"""""""""""""""""$$$
14:28:59.902 -> PullPayload: none
14:28:59.902 -> t=1, h=1 """"""""""""""""""""$$$


 */
    }
};

#endif
