#include <udt.h>
#include <ccc.h>

class CTCP: public CCC
{
public:
   void init()
   {
      m_bSlowStart = true;
      m_issthresh = 83333;

      m_dPktSndPeriod = 0.0;
      m_dCWndSize = 2.0;

      setACKInterval(2);
      setRTO(1000000);
   }

   virtual void onACK(const int& ack)
   {
      if (ack == m_iLastACK)
      {
         if (3 == ++ m_iDupACKCount)
            DupACKAction();
         else if (m_iDupACKCount > 3)
            m_dCWndSize += 1.0;
         else
            ACKAction();
      }
      else
      {
         if (m_iDupACKCount >= 3)
            m_dCWndSize = m_issthresh;

         m_iLastACK = ack;
         m_iDupACKCount = 1;

         ACKAction();
      }
   }

   virtual void onTimeout()
   {
      m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_bSlowStart = true;
      m_dCWndSize = 2.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_bSlowStart)
      {
         m_dCWndSize += 1.0;

         if (m_dCWndSize >= m_issthresh)
            m_bSlowStart = false;
      }
      else
         m_dCWndSize += 1.0/m_dCWndSize;
   }

   virtual void DupACKAction()
   {
      m_bSlowStart = false;

      m_issthresh = getPerfInfo()->pktFlightSize / 2;
      if (m_issthresh < 2)
         m_issthresh = 2;

      m_dCWndSize = m_issthresh + 3;
   }

protected:
   int m_issthresh;
   bool m_bSlowStart;

   int m_iDupACKCount;
   int m_iLastACK;
};

/*****************************************************************************
BiC TCP congestion control
Reference:
Lisong Xu, Khaled Harfoush, and Injong Rhee, "Binary Increase Congestion 
Control for Fast Long-Distance Networks", INFOCOM 2004.
*****************************************************************************/

class CBiCTCP: public CTCP
{
public:
   CBiCTCP()
   {
      m_dMaxWin = m_iDefaultMaxWin;
      m_dMinWin = m_dCWndSize;
      m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;

      m_dSSCWnd = 1.0;
      m_dSSTargetWin = m_dCWndSize + 1.0;
   }

protected:
   virtual void ACKAction()
   {
      if (m_dCWndSize < m_iLowWindow)
      {
         m_dCWndSize += 1/m_dCWndSize;
         return;
      }

      if (!m_bSlowStart)
      {
         if (m_dTargetWin - m_dCWndSize < m_iSMax)
            m_dCWndSize += (m_dTargetWin - m_dCWndSize)/m_dCWndSize;
         else
            m_dCWndSize += m_iSMax/m_dCWndSize;

         if (m_dMaxWin > m_dCWndSize)
         {
            m_dMinWin = m_dCWndSize;
            m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;
         }
         else
         {
            m_bSlowStart = true;
            m_dSSCWnd = 1.0;
            m_dSSTargetWin = m_dCWndSize + 1.0;
            m_dMaxWin = m_iDefaultMaxWin;
         }
      }
      else
      {
         m_dCWndSize += m_dSSCWnd/m_dCWndSize;
         if(m_dCWndSize >= m_dSSTargetWin)
         {
            m_dSSCWnd *= 2;
            m_dSSTargetWin = m_dCWndSize + m_dSSCWnd;
         }
         if(m_dSSCWnd >= m_iSMax)
            m_bSlowStart = false;
      }        
   }

   virtual void DupACKAction()
   {
      if (m_dCWndSize <= m_iLowWindow)
         m_dCWndSize *= 0.5;
      else
      {
         m_dPreMax = m_dMaxWin;
         m_dMaxWin = m_dCWndSize;
         m_dCWndSize *= 0.875;
         m_dMinWin = m_dCWndSize;

         if (m_dPreMax > m_dMaxWin)
         {
            m_dMaxWin = (m_dMaxWin + m_dMinWin) / 2;
            m_dTargetWin = (m_dMaxWin + m_dMinWin) / 2;
         }
      }
   }

private:
   static const int m_iLowWindow = 38;
   static const int m_iSMax = 32;
   static const int m_iSMin = 1;
   static const int m_iDefaultMaxWin = 1 << 29;

   double m_dMaxWin;
   double m_dMinWin;
   double m_dPreMax;
   double m_dTargetWin;
   double m_dSSCWnd;
   double m_dSSTargetWin;
};

class CUDPBlast: public CCC
{
public:
   CUDPBlast()
   {
      printf(" CUDPBlast\n");
      // m_dPktSndPeriod = 1000000; 
      m_dPktSndPeriod = 0.0;
      m_dCWndSize = 83333.0; 
      // m_dCWndSize = 2.0;
   }

public:
   void setRate(double mbps)
   {
      m_dPktSndPeriod = (m_iMSS * 8.0) / mbps;
   }
};
