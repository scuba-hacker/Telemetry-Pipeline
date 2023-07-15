#include "TelemetryPipeline.h"

#include <cstring>    // for memset, memcpy

// class BlockHeader

uint16_t BlockHeader::s_maxPayloadSize = BlockHeader::s_defaultMaxPayloadSize;

void BlockHeader::s_overrideMaxPayloadSize(const uint16_t newMaxPayloadSize)
{
  s_maxPayloadSize = newMaxPayloadSize;
}

BlockHeader::BlockHeader(uint8_t* buffer) : 
    m_payloadId(s_noInitPayloadId), m_PayloadSize(0),m_buffer(buffer)
{      
  initBuffer();
}

BlockHeader::BlockHeader() : 
    m_payloadId(s_noInitPayloadId),m_PayloadSize(s_noInitPayloadSize), m_buffer(NULL)
{}

void BlockHeader::initBuffer()
{
  memset(m_buffer, s_noInitByte, s_maxPayloadSize);
}

bool BlockHeader::isBlockValid() const
{
  return (m_payloadId != s_noInitPayloadId &&
          m_PayloadSize != s_noInitPayloadSize && m_PayloadSize <= s_maxPayloadSize &&
          m_buffer != NULL);
}    

bool BlockHeader::setPayloadId(const uint32_t id)
{
  if (id != s_noInitPayloadId)
  {
    m_payloadId = id; 
    return true;
  }
  else
  {
    return false;
  }
}

bool BlockHeader::setPayloadSize(const uint16_t newPayloadSize) 
{ 
  if (newPayloadSize <= BlockHeader::s_getMaxPayloadSize())
  {
    m_PayloadSize = newPayloadSize;
    return true;
  }
  else
  {
    return false;
  }
}

bool BlockHeader::resetPayload()
{
  m_PayloadSize = 0;
  m_payloadId = 0;
  if (m_buffer != NULL)
  {
    initBuffer();
    return true;
  }
  else
    return false;
}

/*
bool BlockHeader::set(const uint32_t id, const uint16_t PayloadSize)
{  
  if (id != s_noInitPayloadId && PayloadSize <= s_maxPayloadSize)
  {
    m_payloadId = id;
    m_PayloadSize = PayloadSize;
    return true;
  }
  else
  {  
    return false;
  }
}
*/

uint8_t* BlockHeader::getBuffer(uint16_t& maxPayloadSize)
{
  maxPayloadSize = s_maxPayloadSize;
  return m_buffer;
}

// class Pipeline

Pipeline::Pipeline()
{
  m_blockBuffer = NULL;
  m_size = 0;
  m_numberOfBlocks = 0;
  m_blockLength = 0;
  m_headers = NULL;

  m_maxBlockBufferMemoryUsageKB = m_maxBlockBufferMemoryUsageBytes = m_pipelineBlockCount = 0;
}

Pipeline::~Pipeline()
{
  if (m_blockBuffer)
    free(m_blockBuffer);

  delete [] m_headers;
}

bool Pipeline::init(const uint16_t maxBlockBufferMemoryUsageKB,const uint16_t maxBlockBufferMemoryUsageBytesRemainder)
{
  m_maxBlockBufferMemoryUsageKB = maxBlockBufferMemoryUsageKB;
  m_maxBlockBufferMemoryUsageBytes = m_maxBlockBufferMemoryUsageKB * 1024 + maxBlockBufferMemoryUsageBytesRemainder;
  
  m_pipelineBlockCount = m_maxBlockBufferMemoryUsageBytes / BlockHeader::s_getMaxPayloadSize();

  if (m_blockBuffer)
    free(m_blockBuffer);

  if (m_pipelineBlockCount == 0)    // maxPayloadSize is larger than buffer size
  {
    return false;
  }

  m_blockBuffer = (uint8_t*) malloc(m_maxBlockBufferMemoryUsageBytes+1);
  m_blockBuffer[m_maxBlockBufferMemoryUsageBytes] = NULL;   // allows for println output of buffer when testing (using no embedded NULLs)

  if (m_blockBuffer == NULL)
    return false;

  memset(m_blockBuffer,s_noInitPipelineBufferByte,m_maxBlockBufferMemoryUsageBytes);

  if (m_headers)
    delete [] m_headers;
  
  m_headers = new BlockHeader[m_pipelineBlockCount];
  
  if (m_headers == NULL)
    return false;
      
  m_size = m_maxBlockBufferMemoryUsageBytes;
  m_blockLength = BlockHeader::s_getMaxPayloadSize();
  m_numberOfBlocks = m_pipelineBlockCount;

  // initialise all BlockHeaders with buffer pointers
  for (int i=0; i<m_numberOfBlocks; i++)
  {
    m_headers[i] = BlockHeader(getBlockBuffer(i));
  }
        
  return true;
}

uint8_t* Pipeline::getBlockBuffer(const uint16_t index)
{
  return m_blockBuffer + (index * m_blockLength);
}

BlockHeader& Pipeline::operator[](const uint16_t index)
{
  return m_headers[index];
}

const BlockHeader& Pipeline::operator[](const uint16_t index) const
{
  return m_headers[index];
}

// class TelemetryPipeline

TelemetryPipeline::TelemetryPipeline()
{
  initialiseMemberVariables(); 
}

bool TelemetryPipeline::init(long unsigned int (*fn_millis)(void), 
							const uint16_t maxBlockBufferMemoryUsageKB, 
							const uint16_t maxBlockBufferMemoryUsageBytesRemainder)
{
  initialiseMemberVariables();
  m_fn_millis = fn_millis;
  
  return m_pipeline.init(maxBlockBufferMemoryUsageKB,maxBlockBufferMemoryUsageBytesRemainder);
}

void TelemetryPipeline::initialiseMemberVariables()
{
    m_headBlockIndex = m_tailBlockIndex = m_uniquePayloadId = m_pipelineLength = m_longestPipelineLength = 0;
	m_fn_millis = NULL; 
}

// 1. When a message is ready for encoding and storing into a Block buffer, this function
//    is called to get the BlockHeader for that block. The caller then has to
//      (a) populate the block m_payloadBuffer with the message bytes.
//      (b) set the block m_PayloadSize.
BlockHeader TelemetryPipeline::getHeadBlockForPopulating()
{
  BlockHeader head = m_pipeline[m_headBlockIndex];
  head.resetPayload();
  return head;
}

// 2. After the block has been populated with message payload bytes and payload length,
//    caller commits the BlockHeader. The BlockHeader retrieved from getHeadBlockForPopulating()
//    is turned around and passed back here. The object now has the byte buffer populated and payload length set != 0.
bool TelemetryPipeline::commitPopulatedHeadBlock(BlockHeader head, bool& pipelineFull )
{      
  if (head.getPayloadSize() == 0)
  {
    return false;
  }
  else
  {
    head.setPayloadId(m_uniquePayloadId++);
    m_pipeline[m_headBlockIndex] = head;
    advanceHeadBlockIndex(pipelineFull);
    
    if(m_headBlockIndex==m_tailBlockIndex)
      advanceTailBlockIndex();  // make space for the next head 

	m_lastHeadCommitTime = m_fn_millis();

    return true;
  }
}

// 3. After a new message has been pushed into the pipeline, the caller then calls
//    this method to get then next message to convert and push to the internet.
//    If the caller successfully sends the message then the tailBlockCommitted()
//    method must be called.
bool TelemetryPipeline::pullTailBlock(BlockHeader& header)
{
  if (pipelineEmpty())
    return false;
  else
  {
    header = m_pipeline[m_tailBlockIndex];
    return true;
  }
}

// 4. Once the tail block has been successfully uploaded to the internet, the caller must
//    call this method.
void TelemetryPipeline::tailBlockCommitted()
{
  advanceTailBlockIndex();
  m_lastTailCommitTime = m_fn_millis();
}

void TelemetryPipeline::advanceHeadBlockIndex(bool& tailBlockDropped)
{
  tailBlockDropped = false;
  
//  if (pipelineFull())
//  {
//    getNextBlockIndex(m_tailBlockIndex);  // drop the oldest message
//    tailBlockDropped = true;
//  }

  getNextBlockIndex(m_headBlockIndex);

  m_pipelineLength = getPipelineLength();
  
  if (m_pipelineLength > m_longestPipelineLength)
    m_longestPipelineLength = m_pipelineLength;
}

// returns false if no more messages in pipeline.
bool TelemetryPipeline::advanceTailBlockIndex()
{
  m_pipeline[m_tailBlockIndex].resetPayload();    // clear the current tail
  
  getNextBlockIndex(m_tailBlockIndex);
  
  m_pipelineLength = getPipelineLength();

  return !pipelineEmpty();
}

void TelemetryPipeline::getNextBlockIndex(uint16_t& blockIndex) const
{
  blockIndex = (blockIndex + 1) % m_pipeline.getPipelineBlockCount();
}

bool TelemetryPipeline::pipelineEmpty() const
{
  return (m_headBlockIndex == m_tailBlockIndex);
}

bool TelemetryPipeline::pipelineFull() const
{
  uint16_t temp = m_headBlockIndex;
  getNextBlockIndex(temp);
  return (m_tailBlockIndex == temp);
}

uint16_t TelemetryPipeline::getPipelineLength() const
{
  if (m_headBlockIndex >= m_tailBlockIndex)
    return m_headBlockIndex - m_tailBlockIndex;
  else
	return m_pipeline.getPipelineBlockCount() - m_tailBlockIndex + m_headBlockIndex;
}

bool TelemetryPipeline::isPipelineDraining() const
{
	if (getPipelineLength() == 0)
		return true;
	else
	{
		// if no tail commits have been performed in the last x milliseconds then not draining
		return (m_fn_millis() < m_lastTailCommitTime + TelemetryPipeline::s_pipelineNotDrainingPeriod);
	}
}

