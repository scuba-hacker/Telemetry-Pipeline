#ifndef TELEMETRY_PIPELINE_H
#define TELEMETRY_PIPELINE_H

#include <stdint.h>

class BlockHeader
{
  private:
    static uint16_t s_maxPayloadSize;

  public:
    static const uint16_t s_defaultMaxPayloadSize = 200;

  private:
    static const uint16_t s_noInitPayloadSize = 0xFFFF;
    static const uint32_t s_noInitPayloadId = 0xFFFFFFFF;
    static const uint8_t  s_noInitByte = 0x22;    // character #

  public:
    static uint8_t s_getNoInitByte() { return s_noInitByte; }

    static void s_overrideMaxPayloadSize(const uint16_t newMaxPayloadSize);
	static uint16_t s_getMaxPayloadSize() { return s_maxPayloadSize; }


private:
    uint16_t       m_PayloadSize;   // in bytes
    uint32_t       m_payloadId;     // ever-incrementing payloadId
    uint8_t*       m_buffer;

	uint16_t	   m_roundedUpPayloadSize;		// to allow for 2/4/8 byte word boundaries

  public:

    void initBuffer();

    BlockHeader();
    BlockHeader(uint8_t* buffer);

    bool     isBlockValid() const;

    uint32_t getPayloadId()     const { return m_payloadId; }
    uint16_t getPayloadSize() const { return m_PayloadSize; }
    uint8_t* getBuffer(uint16_t& maxPayloadSize);
    bool     setPayloadSize(const uint16_t newPayloadSize);
    uint16_t getMaxPayloadSize() const { return s_maxPayloadSize; }

    bool setPayloadId(const uint32_t id);

    void resetPayload();

	void setRoundedUpPayloadSize(const uint16_t rounded) { m_roundedUpPayloadSize = rounded; }

	uint16_t getRoundedUpPayloadSize() const { return m_roundedUpPayloadSize; }

    bool operator==(const BlockHeader& b)
    {
      return (b.m_PayloadSize == m_PayloadSize &&
              b.m_payloadId == m_payloadId &&
              b.m_buffer == m_buffer &&
			  b.m_roundedUpPayloadSize == m_roundedUpPayloadSize);
    }

//    bool set(const uint32_t id, const uint16_t PayloadSize); 
};

// 100KB of heap for Pipeline, 512 blocks of 200 bytes.
// 512 messages, if generated on per 15 seconds basis instead of twice per second gives two hours capacity without contact with Qubitro/internet.
class Pipeline
{
  public:

    static const uint16_t s_defaultMaxBlockBufferMemoryUsageKB = 100;

  private:
    uint16_t m_maxBlockBufferMemoryUsageKB;
    uint32_t m_maxBlockBufferMemoryUsageBytes;
    uint16_t m_pipelineBlockCount;

    uint8_t* m_blockBuffer;
    uint32_t m_size;
    uint16_t m_numberOfBlocks;
    uint16_t m_blockLength;

    BlockHeader* m_headers;

    static const uint8_t s_noInitPipelineBufferByte = 0x24; // $ character

    Pipeline(const Pipeline& a) {}
    Pipeline& operator=(const Pipeline& a) {return *this;}


  public:
    Pipeline();
    ~Pipeline();

    bool init(const uint16_t maxBlockBufferMemoryUsageKB = Pipeline::s_defaultMaxBlockBufferMemoryUsageKB,
              const uint16_t maxBlockBufferMemoryUsageBytesRemainder = 0);

    void teardown();

    uint8_t* getBlockBuffer(const uint16_t index);

    uint16_t getPipelineBlockCount() const { return m_pipelineBlockCount; }
    uint16_t getPipelineMaxBufferMemBytes() const { return m_maxBlockBufferMemoryUsageBytes; }

    uint8_t* getEntireBuffer(uint32_t& size)
    {
      size = m_size;
      return m_blockBuffer;
    }

    BlockHeader& operator[](const uint16_t index);
    const BlockHeader& operator[](const uint16_t index) const;
};

class TelemetryPipeline
{
  private:
	const static uint32_t s_pipelineNotDrainingPeriod = 10000;	// 10 seconds

  private:
    uint16_t m_headBlockIndex;
    uint16_t m_tailBlockIndex;
    uint32_t m_uniquePayloadId;
    uint16_t m_pipelineLength;
    uint16_t m_longestPipelineLength;

	uint32_t m_lastHeadCommitTime;
	uint32_t m_lastTailCommitTime;

    Pipeline m_pipeline;

    TelemetryPipeline(const TelemetryPipeline& a) {}
    TelemetryPipeline& operator=(const TelemetryPipeline& a) {return *this;}

    void advanceHeadBlockIndex(bool& tailBlockDropped);

    // returns false if no more messages in pipeline.
    bool advanceTailBlockIndex();

    void getNextBlockIndex(uint16_t& blockIndex) const;

	long unsigned int (*m_fn_millis)(void);

  public:
    TelemetryPipeline();

    void initialiseMemberVariables();

    bool init(long unsigned int (*fn_millis)(void), const uint16_t maxBlockBufferMemoryUsageKB = Pipeline::s_defaultMaxBlockBufferMemoryUsageKB,
              const uint16_t maxBlockBufferMemoryUsageBytesRemainder = 0);

    void teardown()
    {
        m_pipeline.teardown();
    }

    uint8_t* getEntireBuffer(uint32_t& size)
    {
      return m_pipeline.getEntireBuffer(size);
    }

    // 1. When a message is ready for encoding and storing into a Block buffer, this function
    //    is called to get the BlockHeader for that block. The caller then has to
    //      (a) populate the block m_payloadBuffer with the message bytes.
    //      (b) set the block m_PayloadSize.
    BlockHeader getHeadBlockForPopulating();

    // 2. After the block has been populated with message payload bytes and payload length,
    //    caller commits the BlockHeader. The BlockHeader retrieved from getHeadBlockForPopulating()
    //    is turned around and passed back here. The object now has the byte buffer populated and payload length set != 0.
    bool commitPopulatedHeadBlock(BlockHeader head, bool& pipelineFull );

    // 3. After a new message has been pushed into the pipeline, the caller then calls
    //    this method to get then next message to convert and push to the internet.
    //    If the caller successfully sends the message then the tailBlockCommitted()
    //    method must be called.
    bool pullTailBlock(BlockHeader& header);

    // 4. Once the tail block has been successfully uploaded to the internet, the caller must
    //    call this method.
    void tailBlockCommitted();

    bool pipelineEmpty() const;
    bool pipelineFull() const;
    uint16_t getPipelineLength() const;
    uint16_t getMaximumDepth() const     { return m_longestPipelineLength; }
    uint16_t getMaximumPipelineLength() const { return m_pipeline.getPipelineBlockCount();}

    uint32_t getCurrentPayloadId() const { return m_uniquePayloadId; }
    uint16_t getHeadBlockIndex() const { return m_headBlockIndex; }
    uint16_t getTailBlockIndex() const { return m_tailBlockIndex; }

    bool isPipelineDraining() const;

    // Stub functions for compatibility with TelemetryFlashManager
    bool performPowerOnSelfTest(bool auto_repair = true) { return true; }
    bool performExtendedDiagnostics()  { return true; }
    bool performDeepSectorValidation()  { return true; }
    bool performPowerLossRecoveryTest()  { return true; }
};
#endif
