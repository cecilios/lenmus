//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2009 LenMus project
//
//    This program is free software; you can redistribute it and/or modify it under the
//    terms of the GNU General Public License as published by the Free Software Foundation,
//    either version 3 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this
//    program. If not, see <http://www.gnu.org/licenses/>.
//
//    For any comment, suggestion or feature request, please contact the manager of
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "Chord.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "Chord.h"
#include "Conversion.h"
#include "../ldp_parser/AuxString.h"
#include "../exercises/Generators.h"
#include "../score/KeySignature.h"
#include "../score/FiguredBass.h"

//access to error's logger
#include "../app/Logger.h"
extern lmLogger* g_pLogger;


static wxString m_sChordName[ect_Max];
static bool m_fStringsInitialized = false;

// AWARE: Array indexes are in correspondence with enum lmEChordType
// - intervals are from root note
//      number + type:   m=minor, M=major, p=perfect, a=augmented, d=diminished
#define lmNIL   lmNULL_FIntval
//TODO: (CDF) consider to improve the creation of this table
//        for example handle also intervals above 4 (CSG: ??? There is no problem in coding other intervals! What do you refer to? 
//TODO: (CDF) consider to separate the static information of this table: nNumNotes, nNumIntervals, nIntervals
//        from the dynamically calculated. (CSG: ? This table only contains staic information! What do you refer to?
//      Idea: tData should belong to a class/struct more specific than lmChordInfo. Create a new one.
static lmChordInfo tData[ect_Max] = {
//  { NumNotes, NumIntervals, NumInversions, FifthElided { intervals } }
    { 3, 2, 0, 0, false,{ lm_M3, lm_p5, lmNIL }},      //MT        - MajorTriad
    { 3, 2, 0, 0, false,{ lm_m3, lm_p5, lmNIL }},      //mT        - MinorTriad
    { 3, 2, 0, 0, false,{ lm_M3, lm_a5, lmNIL }},      //aT        - AugTriad
    { 3, 2, 0, 0, false,{ lm_m3, lm_d5, lmNIL }},      //dT        - DimTriad
    { 3, 2, 0, 0, false,{ lm_p4, lm_p5, lmNIL }},      //I,IV,V    - Suspended_4th
    { 3, 2, 0, 0, false,{ lm_M2, lm_p5, lmNIL }},      //I,II,V    - Suspended_2nd
    { 4, 3, 0, 0, false,{ lm_M3, lm_p5, lm_M7 }},      //MT + M7   - MajorSeventh
    { 4, 3, 0, 0, false,{ lm_M3, lm_p5, lm_m7 }},      //MT + m7   - DominantSeventh
    { 4, 3, 0, 0, false,{ lm_m3, lm_p5, lm_m7 }},      //mT + m7   - MinorSeventh
    { 4, 3, 0, 0, false,{ lm_m3, lm_d5, lm_d7 }},      //dT + d7   - DimSeventh
    { 4, 3, 0, 0, false,{ lm_m3, lm_d5, lm_m7 }},      //dT + m7   - HalfDimSeventh
    { 4, 3, 0, 0, false,{ lm_M3, lm_a5, lm_M7 }},      //aT + M7   - AugMajorSeventh
    { 4, 3, 0, 0, false,{ lm_M3, lm_a5, lm_m7 }},      //aT + m7   - AugSeventh
    { 4, 3, 0, 0, false,{ lm_m3, lm_p5, lm_M7 }},      //mT + M7   - MinorMajorSeventh
    { 4, 3, 0, 0, false,{ lm_M3, lm_p5, lm_M6 }},      //MT + M6   - MajorSixth
    { 4, 3, 0, 0, false,{ lm_m3, lm_p5, lm_M6 }},      //mT + M6   - MinorSixth
    { 3, 3, 0, 0, false,{ lm_M3, lm_a4, lm_a6 }},      //          - AugSixth
};

// Function to get a the pitch relative to key signature
// TODO: consider: global function?  where should it be located?
wxString NoteId(lmNote &tNote)
{
    // Get pitch relative to key signature
    lmFPitch fp = FPitch(tNote.GetAPitch());
    lmKeySignature* pKey = tNote.GetApplicableKeySignature();
    lmEKeySignatures nKey = (pKey ? pKey->GetKeyType() : earmDo);
    wxString sPitch = FPitch_ToRelLDPName(fp, nKey);
    return sPitch;
}

 void SortChordNotes(int nNumNotes, lmNote** pInpChordNotes)
{
    wxASSERT(nNumNotes < lmNOTES_IN_CHORD);
    // Classic Bubble sort
    int nCount, fSwapDone;
    lmNote* auxNote;
    do
    {
        fSwapDone = 0;
        for (nCount = 0; nCount < nNumNotes - 1; nCount++)
        {
            wxASSERT(pInpChordNotes[nCount] != NULL);
            wxASSERT(pInpChordNotes[nCount+1] != NULL);
            if (pInpChordNotes[nCount]->GetFPitch() > pInpChordNotes[nCount+1]->GetFPitch() )
            {
	            auxNote = pInpChordNotes[nCount];
	            pInpChordNotes[nCount] = pInpChordNotes[nCount + 1];
	            pInpChordNotes[nCount + 1] = auxNote;
	            fSwapDone = 1;
            }
        }
    }while (fSwapDone);
}
void  GetIntervalsFromNotes(int nNumNotes, lmNote** pInpChordNotes, lmChordInfo* tOutChordInfo)
{
    wxASSERT(nNumNotes < lmNOTES_IN_CHORD);
    wxASSERT(pInpChordNotes != NULL);
    wxASSERT(tOutChordInfo != NULL);
    wxASSERT(pInpChordNotes[0] != NULL);

    lmFIntval fpIntv;
    int nCurrentIntvIndex = 0;
    int nExistingIntvIndex = 0;
    for (int nCount = 1; nCount < nNumNotes; nCount++)
    {
        wxASSERT(pInpChordNotes[nCount] != NULL);
        fpIntv = (lmFIntval) (pInpChordNotes[nCount]->GetFPitch() - pInpChordNotes[0]->GetFPitch());

        if (fpIntv >= lm_p8)
        {
              fpIntv = fpIntv % lm_p8;
        }
#ifdef __WXDEBUG__
        wxLogMessage(_T("[GetIntervalsFromNotes note %d: %d  note 0: %d] INTERVAL: %d")
            , nCount, pInpChordNotes[nCount]->GetFPitch(), pInpChordNotes[0]->GetFPitch(), fpIntv);
#endif
        // Update chord interval information
        for (nExistingIntvIndex=0; nExistingIntvIndex<nCurrentIntvIndex; nExistingIntvIndex++)
        {
            if (tOutChordInfo->nIntervals[nExistingIntvIndex] == fpIntv)
                break; // interval already exists
        }
        if (nExistingIntvIndex < nCurrentIntvIndex)
        {
            wxLogMessage(_(" Interval %d: IGNORED, already in %d")
            , fpIntv, nExistingIntvIndex);
        }
        else
        {
            if (fpIntv == 0)
            {
                // Ignored 0 Interval
            }
            else
            {
                // Add interval
                tOutChordInfo->nIntervals[nCurrentIntvIndex] = fpIntv;
                nCurrentIntvIndex++;
            }
        }
    }

    tOutChordInfo->nNumNotes = nNumNotes;
    tOutChordInfo->nNumIntervals = nCurrentIntvIndex;

    // Sort Intervals
    //  Classic bubble sort
    int fSwapDone;
    lmFIntval auxIntv;
    do
    {
        fSwapDone = 0;
        for (int nCount = 0; nCount < tOutChordInfo->nNumIntervals - 1; nCount++)
        {
            if (tOutChordInfo->nIntervals[nCount] > tOutChordInfo->nIntervals[nCount+1] )
            {
                auxIntv = tOutChordInfo->nIntervals[nCount];
                tOutChordInfo->nIntervals[nCount] = tOutChordInfo->nIntervals[nCount + 1];
                tOutChordInfo->nIntervals[nCount + 1] = auxIntv;
                fSwapDone = 1;
            }
        }
    }while (fSwapDone);


    // Set the non-used intervals to  NULL
    for (int i=tOutChordInfo->nNumIntervals; i<lmINTERVALS_IN_CHORD; i++)
    {
      tOutChordInfo->nIntervals[i] = lmNULL_FIntval;
    }

}

lmEChordType GetChordTypeFromIntervals(lmChordInfo& tChordInfo, bool fAllowFifthElided )
{
    // Search the chord type that matches the specified intervals
    // For each item, it must match:
    //   number of intervals
    //   every interval
    for (int nIntv = 0; nIntv < ect_Max; nIntv++)
    {
        if ( tChordInfo.nNumIntervals == tData[nIntv].nNumIntervals)
        {
            bool fDifferent = false;
            for (int i = 0; i < tChordInfo.nNumIntervals && !fDifferent; i++)
            {
                if (tChordInfo.nIntervals[i] != tData[nIntv].nIntervals[i])
                  fDifferent = true;
            }
            if (!fDifferent)
              return (lmEChordType) nIntv;  // found matching item
        }
    }

    if ( fAllowFifthElided )
    {
        // Special case: Fifth elided
        //  TODO: improve?  ONLY ONE POSSIBLE CASE: SECOND INTERVAL MISSING
        for (int nIntv = 0; nIntv < ect_Max; nIntv++)
        {
            if ( tChordInfo.nNumIntervals == tData[nIntv].nNumIntervals - 1)
            {
                bool fDifferent = false;
                // Interval 0 must match
                // Interval 1 is not checked!
                // For the rest... what TODO:??
                if (tChordInfo.nIntervals[0] != tData[nIntv].nIntervals[0])
                      fDifferent = true;
    #ifdef __WXDEBUG__
                else
                     wxLogMessage(_T(" Check fifth ellided, %d intv,  %d == %d, ITEM:%d")
                        , tChordInfo.nNumIntervals, tChordInfo.nIntervals[0], tData[nIntv].nIntervals[0], nIntv );
    #endif
                // For the rest... what TODO:??  for the moment: they must also be the same
                for (int i = 2; i < tData[nIntv].nNumIntervals && !fDifferent; i++)
                {
                    if (tChordInfo.nIntervals[i] != tData[nIntv].nIntervals[i+1])
                    {
                      fDifferent = true;
                    }
                }
                if (!fDifferent)
                  return (lmEChordType) nIntv;  // found matching item
            }
        }
    }

    return lmINVALID_CHORD_TYPE; // invalid chord
}

// Perform n inversions/reversions over a chord with any number of intervals
//   Inversion: the lowest note is increased one octave.
//   Reversion: the highest note is reduced one octave.
// Return: number of inversions actually performed
int DoInversionsToChord( lmChordInfo* pInOutChordInfo, int nNumTotalInv)
{
    // The algoritm is simple; the inversion is performed by updating the intervals as follows:
    //  INCREMENT inversion:
    //   NewInterval[i] = OldInterval[i+1] - OldInterval[0]; // when i < N
    //   NewInterval[N] = lm_p8 - OldInterval[0]; // when i = N
    //  REVERSION (decrement inversion):
    //   NewInterval[0] = lm_p8 - OldInterval[N]; // when i = 0
    //   NewInterval[i] = OldInterval[i-1] + NewInterval[0]; // when i > 0

    lmFIntval fpIntv = 0;

    // Create a copy of the input
    lmChordInfo tInChordInfo;
    tInChordInfo =  *pInOutChordInfo;

    if (   pInOutChordInfo == NULL
        || tInChordInfo.nNumNotes > (lmFIntval)lmINTERVALS_IN_CHORD+1
        || tInChordInfo.nNumIntervals > tInChordInfo.nNumNotes-1
        || tInChordInfo.nNumIntervals < 1
        )
    {
        if (pInOutChordInfo == NULL)
           wxLogMessage(_T(" DoInversionsToChord: NULL DATA"));
        else
           wxLogMessage(_T(" DoInversionsToChord: wrong data Num Notes %d, Num Intvervals %d")
            ,tInChordInfo.nNumNotes, tInChordInfo.nNumIntervals);
        return 0;
    }

#ifdef __WXDEBUG__
        wxLogMessage(_T(" Before inversion: NumNotes %d, NumIntv %d, i0:%d i1:%d i2:%d")
            , tInChordInfo.nNumNotes, tInChordInfo.nNumIntervals
            , tInChordInfo.nIntervals[0], tInChordInfo.nIntervals[1],tInChordInfo.nIntervals[2]);
#endif

    // number of notes and intervals remains unchanged
    pInOutChordInfo->nNumNotes = tInChordInfo.nNumNotes;
    pInOutChordInfo->nNumIntervals = tInChordInfo.nNumIntervals;

    // aware:
    //  nNumNotes: number of notes
    //  nNumIntervals: number of intervals
    //  nNumIntervals-1: index of last inteval (N)
    //  nNumIntervals-2: index of penultimate interval

    if ( nNumTotalInv > 0)
    {
        for ( int nInv=0; nInv<nNumTotalInv; nInv++)
        {
            int i=0;
            for ( i=0; i<tInChordInfo.nNumIntervals-1; i++)
            {
                pInOutChordInfo->nIntervals[i] = tInChordInfo.nIntervals[i+1] - tInChordInfo.nIntervals[0];
            }
            pInOutChordInfo->nIntervals[tInChordInfo.nNumIntervals-1] = lm_p8 - tInChordInfo.nIntervals[0];

        #ifdef __WXDEBUG__
                wxLogMessage(_T(" After adding inversion %d : NumNotes %d, NumIntv %d, i0:%d i1:%d i2:%d")
                    ,nInv,tInChordInfo.nNumNotes,tInChordInfo.nNumIntervals
                    , tInChordInfo.nIntervals[0], tInChordInfo.nIntervals[1],tInChordInfo.nIntervals[2]);
        #endif
        }
    }
    else if ( nNumTotalInv < 0)
    {
        for ( int nInv=0; nInv<-nNumTotalInv; nInv++)
        {
            int i=0;
            pInOutChordInfo->nIntervals[0] = lm_p8 - tInChordInfo.nIntervals[tInChordInfo.nNumIntervals-1];
            for ( i=1; i<tInChordInfo.nNumIntervals; i++)
            {
                pInOutChordInfo->nIntervals[i] = tInChordInfo.nIntervals[i-1] + pInOutChordInfo->nIntervals[0];
            }
        #ifdef __WXDEBUG__
                wxLogMessage(_T(" After reducing inversion %d : NumNotes %d, NumIntv %d, i0:%d i1:%d i2:%d")
                    ,nInv,tInChordInfo.nNumNotes,tInChordInfo.nNumIntervals
                    , tInChordInfo.nIntervals[0], tInChordInfo.nIntervals[1],tInChordInfo.nIntervals[2]);
        #endif
        }
    }
    return nNumTotalInv;
}


// Improvement of GetChordTypeFromIntervals: also consider possible inversions
//
//    Algorithm:
//
// Repeat while not valid chord type found and inversions are possible
//    (possible inversions = number of intervals - 1 = number of notes - 2)
//   Search the chord type that matches the specified intervals
//    found: return chord type
//    not found: Do one inversion
//
lmEChordType GetChordTypeAndInversionsFromIntervals( lmChordInfo &tChordInfo)
{
    lmEChordType nType = lmINVALID_CHORD_TYPE;
    int nNumPossibleInversions = tChordInfo.nNumIntervals;
    tChordInfo.nNumInversions = 0;
    do
    {
        nType = GetChordTypeFromIntervals( tChordInfo );

        if (nType != lmINVALID_CHORD_TYPE)
             return nType;

        // Try with inversions
        if (tChordInfo.nNumInversions >= nNumPossibleInversions)
            return lmINVALID_CHORD_TYPE; // invalid chord
        else
        {
            // Remove one inversion from chord
            int nInvOk = DoInversionsToChord(&tChordInfo, -1);
            tChordInfo.nNumInversions++;
        }

    }while ( nType == lmINVALID_CHORD_TYPE );

    return nType;
}



wxString lmChord::ToString()
{
    wxString sRetStr;
    if ( ! this->IsCreated() )
        sRetStr = _("Not recognized");
    else
    {
        int nNumNotes = GetNumNotes();
        // Note that the number of notes and the number of inversions is already in the description from GetNameFull
        sRetStr = wxString::Format(_T(" %s"), GetNameFull().c_str());

        if (m_nElision > 0)
          sRetStr += wxString::Format(_(", %d elisions"), m_nElision);

        sRetStr += _(", Notes: ");

        for (int n=0; n<nNumNotes; n++)
        {
            sRetStr += _T(" ");
            sRetStr += GetPattern(n);
        }
    }
    return sRetStr;
}
void lmChord::Create(lmNote* pRootNote, lmChordInfo* pChordInfo)
{
    lmKeySignature* pKey = pRootNote->GetApplicableKeySignature();
    lmEKeySignatures nKey = (pKey ? pKey->GetKeyType() : earmDo);

    assert(pChordInfo != NULL);

    m_nKey = nKey;

    m_nNumNotes = pChordInfo->nNumIntervals + 1;
    m_nInversion = pChordInfo->nNumInversions;
    m_fpNote[0] = pRootNote->GetFPitch();
    m_nElision = pChordInfo->nFifthElided;

    DoCreateChord(pChordInfo->nIntervals);

}

const bool CONSIDER_5TH_ELIDED = true;

// TODO: consider improvement: TryChordCreation might actually CREATE the ChordManager instead of
//         just trying to do it
//
// Look for notes in the score that make up a valid chord
bool TryChordCreation(int nNumNotes, lmNote** pInpChordNotes, lmChordInfo* tOutChordInfo, wxString &sOutStatusStr)
{
    bool fOk = false;

    sOutStatusStr =  wxString::Format(_("%d notes: ") ,  nNumNotes);
    wxASSERT(pInpChordNotes != NULL);

    tOutChordInfo->Initalize();

    if (nNumNotes < 3)
    {
        sOutStatusStr +=  _(" at least 3 notes are required !");
        return false;
    }

    // Sort notes
    SortChordNotes(nNumNotes, pInpChordNotes);

    // Check whether root not is duplicated
    tOutChordInfo->fRootIsDuplicated = false;
    for (int i=1; i<nNumNotes; i++)
    {
        if ( pInpChordNotes[i]->GetFPitch() == pInpChordNotes[i]->GetFPitch())
            tOutChordInfo->fRootIsDuplicated = true;
    }

    for (int i=0; i<nNumNotes; i++)
    {
        sOutStatusStr +=  wxString::Format(_T(" %s") 
            ,NoteId(*pInpChordNotes[i]).c_str() );
    }

    // Get intervals and Create lmChordInfo from notes
    GetIntervalsFromNotes(nNumNotes, pInpChordNotes, tOutChordInfo);

    for (int i=0; i<tOutChordInfo->nNumIntervals; i++)
    {
        sOutStatusStr +=  wxString::Format(_(", %d intervals")
            , tOutChordInfo->nIntervals[i]);
    }

    lmChordInfo tOriOutChordInfo; // TODO: this might be improved; we need to keep the original to check "fifth elided"
    //(now GetChordTypeAndInversionsFromIntervals changes tOutChordInfo a cause of the inversions)
    // TODO: IDEA: move the check "fifth elided" to a rule
    tOriOutChordInfo.Initalize();
    tOriOutChordInfo = *tOutChordInfo;

    lmEChordType nType = GetChordTypeAndInversionsFromIntervals(*tOutChordInfo);

    // If returned lmINVALID_CHORD_TYPE, the intervals do not make up a valid chord
    // ...but as a last resort: we consider a possible 5th elided
    if (CONSIDER_5TH_ELIDED && nType == lmINVALID_CHORD_TYPE)
    {
        //TODO: consider to improve by placing this inside GetChordTypeAndInversionsFromIntervals
        //TODO: consider to check elision BEFORE GetChordTypeAndInversionsFromIntervals
        //(now GetChordTypeAndInversionsFromIntervals changes tOutChordInfo a cause of the inversions)

        // Try with "fifth elided"
        nType = GetChordTypeFromIntervals( tOriOutChordInfo, true );
        if (nType != lmINVALID_CHORD_TYPE)
        {
    #ifdef __WXDEBUG__
            wxLogMessage(_T(" Checking fifth ellided, type:%d"), nType);
    #endif
            //  TODO: consider to improve by checking that root note is x2 or x3
            //  TODO: consider to improve by checking  other possible elisions
            *tOutChordInfo = tOriOutChordInfo; // TODO: might be improved; we need it to keep the original
            tOutChordInfo->nFifthElided = 1;
        }
    }


    if (nType == lmINVALID_CHORD_TYPE )
    {
    #ifdef __WXDEBUG__
        wxLogMessage(_T(" Invalid chord: Num notes %d, Ell:%d, i0:%d i1:%d i2:%d")
            ,  tOutChordInfo->nNumNotes, tOutChordInfo->nFifthElided, tOutChordInfo->nIntervals[0], tOutChordInfo->nIntervals[1]
        , tOutChordInfo->nIntervals[2] );
    #endif
        fOk = false;
        sOutStatusStr +=  _(", do not make up a valid chord!");
    }
    else
    {
        fOk = true;
        sOutStatusStr +=  _T(", chord OK ");

    }
    return fOk;
}

lmChord::lmChord(lmNote* pRootNote, lmChordInfo &tChordInfo)
{
    this->Initialize(); // call basic constructor for initialization

    m_fRootIsDuplicated = tChordInfo.fRootIsDuplicated;

    // 
    m_nType = GetChordTypeFromIntervals( tChordInfo,  tChordInfo.nFifthElided > 0);
    if ( m_nType == lmINVALID_CHORD_TYPE )
    {
        //TODO: consider to improve the management of invalid chord...
        wxLogMessage(_T(" lmChord Invalid chord: Num notes %d, Num intv %d, 5thEllid %d, i0:%d i1:%d i2:%d")
            , tChordInfo.nNumNotes, tChordInfo.nNumIntervals, tChordInfo.nFifthElided
             , tChordInfo.nIntervals[0], tChordInfo.nIntervals[1], tChordInfo.nIntervals[2]  );
#ifdef __WXDEBUG__
        wxLogMessage(_T(" tData[0]: Num notes %d, i0:%d i1:%d i2:%d")
            ,  tData[0].nNumNotes, tData[0].nIntervals[0], tData[0].nIntervals[1]
             , tData[0].nIntervals[2]  );
#endif
    }
    else
      Create(pRootNote, &tChordInfo);


#ifdef __WXDEBUG__
    wxLogMessage(_T(" CREATED chord: %s"), this->ToString().c_str() );
#endif
}
void lmChord::Initialize()
{
    m_nNumNotes = 0;
    m_nInversion = 0;
    m_nType =  lmINVALID_CHORD_TYPE;
    m_nKey = earmDo; // TODO: consider to initalize with invalid value
    m_nElision = 0;
}



//-------------------------------------------------------------------------------------
// Implementation of lmChord class

lmChord::lmChord()
{
    this->Initialize();
}

lmChord::lmChord(wxString sRootNote, lmEChordType nChordType,
                               int nInversion, lmEKeySignatures nKey)
{
    //creates a chord from its type, the root note, the desired inversion, and the key signature

    Create(sRootNote, nChordType, nInversion, nKey);
}

lmChord::lmChord(wxString sRootNote, lmFiguredBass* pFigBass, 
                               lmEKeySignatures nKey)
    : m_nKey(nKey)
    , m_nInversion(0)
    , m_nNumNotes(0)
    , m_fRootIsDuplicated(false)
{
    //Creates a chord from the root note, the figured bass, and the key signature.

    // Create root note and get its components
    m_fpNote[0] = FPitch(sRootNote);
    int nRootStep = FPitch_Step( m_fpNote[0] );
    int nRootOctave = FPitch_Octave( m_fpNote[0] );
    int nRootAcc = FPitch_Accidentals( m_fpNote[0] );
    m_nNumNotes = 1;      //number of notes in the chord

    //get accidentals for desired key signature
    int nAccidentals[7];
    ::lmComputeAccidentals(nKey, nAccidentals);

    //build the intervals from the root note
    //TODO: Review for intervals equal or greater than one octave
    int nOctave = nRootOctave;
    int nStep = nRootStep;
    int nFirstIntval = 0;
    for (int i=2; i < 8; i++)
    {
        //determine step and octave
        if (++nStep == 8)
        {
            nStep = 0;
            nOctave++;
        }

        //get interval quality
        lmEIntervalQuality nIQ = pFigBass->GetIntervalQuality(i);

        //decide accidentals
        lmFPitch fpNote;
        int nAcc = nAccidentals[nStep];     //accidentals from key signature
        if (pFigBass->IntervalSounds(i))
        {
            lmEIntervalQuality nIntvQuality = pFigBass->GetIntervalQuality(i);
            switch(nIntvQuality)
            {
                case lm_eIM_NotPresent:
                {   //the interval is as implied by key signature
                    break;
                }
                case lm_eIM_AsImplied:
                {   //the interval is as implied by key signature
                    break;
                }
                case lm_eIM_RaiseHalf:
                {   //the interval is raised by half step, relative to the key signature
                    nAcc++;
                    break;
                }
                case lm_eIM_LowerHalf:
                {   //the interval is lowered by half step, relative to the key signature
                    nAcc--;
                    break;
                }
                case lm_eIM_Natural:
                {   //the interval is natural, regardless of the key signature
                    nAcc = 0;
                    break;
                }
                case lm_eIM_Diminished:
                {   //the interval is diminished, regardless of the key signature
                    lmFIntval fi = FIntval_FromType(i, eti_Diminished);
                    fpNote = m_fpNote[0] + fi;
                }
                default:
                    wxASSERT(false);
            }

            //build pitch. It is already built for case lm_eIM_Diminished
            if (nIntvQuality != lm_eIM_Diminished)
                fpNote = FPitch(nStep, nOctave, nAcc);

            //add this note to the chord
            if (m_nNumNotes < lmNOTES_IN_CHORD-1)
                m_fpNote[++m_nNumNotes] = fpNote;
            else
                wxLogMessage(_T("[lmChord::lmChord] Number of notes in a chord exceeded!"));

            //determine the first present interval greater than second
            if (nFirstIntval == 0 && i >= 3)
                nFirstIntval = i;
        }
    }

        //here all chord note are created. Compute chord additional info
    
    //determine inversion type
        //TO_INVESTIGATE:
        // - Is it correct to assume that it is inverted if first 
        //   interval (excluding a possible second) is greater than a third?
        // - Is it correct to assume than it is first inversion if it is a fourth, 
        //   second if it is a sixth, and third if it is a seventh?
    switch(nFirstIntval)
    {
        case 3:     m_nInversion = 0;   break;
        case 4:     m_nInversion = 1;   break;
        case 5:     m_nInversion = 0;   break;      //third elided
        case 6:     m_nInversion = 2;   break;
        case 7:     m_nInversion = 3;   break;
        default:
            m_nInversion = 0;
            wxLogMessage(_T("[lmChord::lmChord] Unknown case. nFirstInterval = %d"),
                         nFirstIntval );
    }

    //determine chord type
    m_nType = ComputeChordType();
}

void lmChord::Create(wxString sRootNote, lmEChordType nChordType,
                            int nInversion, lmEKeySignatures nKey)
{
    //parameter 'nInversion' is encoded as follows:
    //  0 - root position
    //  1 - 1st inversion
    //  2 - 2nd inversion
    //  and so on

    //save parameters
    m_nType = nChordType;
    m_nKey = nKey;
    m_nInversion = nInversion;

    // Create root note data
    m_fpNote[0] = FPitch(sRootNote);

    //get the intervals that form the chord
    lmFIntval nIntval[3], nNewIntv[3];
    nIntval[0] = (lmFIntval)tData[m_nType].nIntervals[0];
    nIntval[1] = (lmFIntval)tData[m_nType].nIntervals[1];
    nIntval[2] = (lmFIntval)tData[m_nType].nIntervals[2];

    //correction for inversions
    if (m_nInversion == 1)
    {
        nNewIntv[0] = nIntval[1] - nIntval[0];

        if (nIntval[2] == lmNIL) {
            nNewIntv[1] = lm_p8 - nIntval[0];   //invert the interval
            nNewIntv[2] = lmNIL;
        }
        else {
            nNewIntv[1] = nIntval[2] - nIntval[0];
            nNewIntv[2] = lm_p8 - nIntval[0];   //invert the interval
        }
    }
    else if (m_nInversion == 2)
    {
        if (nIntval[2] == lmNIL) {
            nNewIntv[0] = lm_p8 - nIntval[1];   //invert the interval
            nNewIntv[1] = lm_p8 + nIntval[0] - nIntval[1];
            nNewIntv[2] = lmNIL;
        }
        else {
            nNewIntv[0] = nIntval[2] - nIntval[1];
            nNewIntv[1] = lm_p8 - nIntval[1];   //invert the interval
            nNewIntv[2] = lm_p8 + nIntval[0] - nIntval[1];
        }
    }
    else if (m_nInversion == 3)
    {
        nNewIntv[0] = lm_p8 - nIntval[2];   //invert the interval
        nNewIntv[1] = nNewIntv[0] + nIntval[0];
        nNewIntv[2] = nNewIntv[0] + nIntval[1];
    }
    if (m_nInversion != 0) {
        nIntval[0] = nNewIntv[0];
        nIntval[1] = nNewIntv[1];
        nIntval[2] = nNewIntv[2];
    }

    m_nNumNotes = tData[m_nType].nNumNotes;
    DoCreateChord(nIntval);

}

void lmChord::Create(wxString sRootNote, wxString sIntervals, lmEKeySignatures nKey)
{
    // save data
    m_nKey = nKey;

    // prepare root note
    m_fpNote[0] = FPitch(sRootNote);

    // extract intervals
    lmFIntval nIntval[lmNOTES_IN_CHORD];
    int iT = 0;                 //index to interval

    int nSize = (int)sIntervals.length();
    int iStart = 0;
    int iEnd = sIntervals.find(_T(','), iStart);
    while (iEnd != (int)wxStringBase::npos && iT < lmNOTES_IN_CHORD-1)
    {
        wxString sIntval = sIntervals.substr(iStart, iEnd-iStart);
        // If first interval is "#" or "b" it refers to the root note
        if (iStart==0 && (sIntval == _T("#") || sIntval == _T("b")))
        {
            // modify root pitch
            if (sIntval == _T("#"))
                m_fpNote[0]++;
            else
                m_fpNote[0]--;
        }
        else
        {
            // convert interval name to value
            nIntval[iT++] = FIntval(sIntval);
        }

        // advance pointers
        iStart = iEnd + 1;
        if (iStart >= nSize) break;
        iEnd = sIntervals.find(_T(','), iStart);

    }
    if (iT < lmNOTES_IN_CHORD-1) {
        wxString sIntval = sIntervals.substr(iStart);
        nIntval[iT++] = FIntval(sIntval);
    }

    m_nNumNotes = iT+1;     //num notes = nim.intervals + 1

    DoCreateChord(nIntval);

}

void lmChord::DoCreateChord(lmFIntval nIntval[])
{
    // root note is created in m_fpNote[0]. Create the remaining notes
    for (int i=1; i < m_nNumNotes; i++) {
        m_fpNote[i] = m_fpNote[0] + nIntval[i-1];
    }

}

lmChord::~lmChord()
{
}

lmFIntval lmChord::GetInterval(int i)
{
    //return the chord interval #i (i = 1 .. m_nNumNotes-1)

    wxASSERT(i > 0 && i < m_nNumNotes);
    return m_fpNote[i] - m_fpNote[i-1];
}

int lmChord::GetNumNotes()
{
    return m_nNumNotes;
}

lmMPitch lmChord::GetMidiNote(int i)
{
    wxASSERT(i < GetNumNotes());
    return FPitch_ToMPitch(m_fpNote[i]);
}

wxString lmChord::GetPattern(int i)
{
    // Returns Relative LDP pattern for note i (0 .. m_nNumNotes-1)
    wxASSERT( i < GetNumNotes());
    return FPitch_ToRelLDPName(m_fpNote[i], m_nKey);
}

wxString lmChord::GetNameFull()
{
    wxString sName = ChordTypeToName( m_nType );

    if ( m_nType != lmINVALID_CHORD_TYPE )
    {
        sName += _T(", ");
        if (m_nInversion == 0)
            sName += _("root position");
        else if (m_nInversion == 1)
            sName += _("1st inversion");
        else if (m_nInversion == 2)
            sName += _("2nd inversion");
        else if (m_nInversion == 3)
            sName += _("3rd inversion");
        else
            wxASSERT(false);    //impossible
    }
    return sName;

}

lmEChordType lmChord::ComputeChordType()
{
    //look for the chord type that matches this chord intervals

    int nNumIntervals = m_nNumNotes - 1;
    for (int nIntv = 0; nIntv < ect_Max; nIntv++)
    {
        if (nNumIntervals == tData[nIntv].nNumIntervals)
        {
            //this entry has the same number of intervals than this chord. Check intervals
            bool fMatch = true;
            for (int i = 0; i < nNumIntervals && fMatch; i++)
            {
                lmFIntval fi = GetInterval(i);
                //look for this interval
                int j;
                for (j = 0; j < nNumIntervals; j++)
                {
                    if (fi == tData[nIntv].nIntervals[j])
                        break;
                }
                fMatch = (j != nNumIntervals);
            }
            if (fMatch)
              return (lmEChordType)nIntv;      //found matching item
        }
    }

    #ifdef __WXDEBUG__
        wxString sIntvals = _T("[lmChord::ComputeChordType] No match found. Intervals: ");
        for (int i = 0; i < nNumIntervals; i++)
            sIntvals += wxString::Format(_T("%d, "), GetInterval(i));
        wxLogMessage(sIntvals);
    #endif

    return lmINVALID_CHORD_TYPE;    //no match found!
}


#ifdef __WXDEBUG__
void lmChord::UnitTests()
{
    int i, j;

    //lmConverter::NoteToBits and lmConverter::NoteBitsToName
    wxLogMessage(_T("[lmChord::UnitTests] Test of lmConverter::NoteToBits() method:"));
    wxString sNote[8] = { _T("a4"), _T("+a5"), _T("--b2"), _T("-a4"),
        _T("+e4"), _T("++f6"), _T("b1"), _T("xc4") };
    lmNoteBits tNote;
    for(i=0; i < 8; i++) {
        if (lmConverter::NoteToBits(sNote[i], &tNote))
            wxLogMessage(_T("Unexpected error in lmConverter::NoteToBits()"));
        else {
            wxLogMessage(_T("Note: '%s'. Bits: Step=%d, Octave=%d, Accidentals=%d, StepSemitones=%d --> '%s'"),
                sNote[i].c_str(), tNote.nStep, tNote.nOctave, tNote.nAccidentals, tNote.nStepSemitones,
                lmConverter::NoteBitsToName(tNote, m_nKey).c_str() );
        }
    }

    //ComputeInterval(): interval computation
    wxString sIntv[8] = { _T("M3"), _T("m3"), _T("p8"), _T("p5"),
        _T("a5"), _T("d7"), _T("M6"), _T("M2") };
    for(i=0; i < 8; i++) {
        for (j=0; j < 8; j++) {
            wxString sNewNote = ComputeInterval(sNote[i], sIntv[j], true, m_nKey);
            wxLogMessage(_T("Note='%s' + Intv='%s' --> '%s'"),
                         sNote[i].c_str(), sIntv[j].c_str(), sNewNote.c_str() );
            wxString sStartNote = ComputeInterval(sNewNote, sIntv[j], false, m_nKey);
            wxLogMessage(_T("Note='%s' - Intv='%s' --> '%s'"),
                         sNewNote.c_str(), sIntv[j].c_str(), sStartNote.c_str() );
        }
    }

    //IntervalCodeToBits and IntervalBitsToCode
    wxLogMessage(_T("[lmChord::UnitTests] Test of IntervalCodeToBits() method:"));
    lmIntvBits tIntv;
    for(i=0; i < 8; i++) {
        if (IntervalCodeToBits(sIntv[i], &tIntv))
            wxLogMessage(_T("Unexpected error in IntervalCodeToBits()"));
        else {
            wxLogMessage(_T("Intv: '%s'. Bits: num=%d, Semitones=%d --> '%s'"),
                sIntv[i].c_str(), tIntv.nNum,tIntv.nSemitones,
                IntervalBitsToCode(tIntv).c_str() );
        }
    }

    ////SubstractIntervals
    //wxLogMessage(_T("[lmChord::UnitTests] Test of SubstractIntervals() method:"));
    //wxString sIntv1[8] = { _T("p5"), _T("p5"), _T("M7"), _T("M6"), _T("m6"), _T("M7"), _T("M6"), _T("p4") };
    //wxString sIntv2[8] = { _T("M3"), _T("m3"), _T("p5"), _T("p5"), _T("a5"), _T("M3"), _T("m3"), _T("M2") };
    //for(i=0; i < 8; i++) {
    //    wxLogMessage(_T("Intv1='%s', intv2='%s' --> dif='%s'"),
    //        sIntv1[i], sIntv2[i], SubstractIntervals(sIntv1[i], sIntv2[i]) );
    //}

    ////AddIntervals
    //wxLogMessage(_T("[lmChord::UnitTests] Test of AddIntervals() method:"));
    //wxString sIntv1[8] = { _T("p5"), _T("p5"), _T("M6"), _T("M3"), _T("M3"), _T("M6"), _T("d4"), _T("p8") };
    //wxString sIntv2[8] = { _T("M3"), _T("m3"), _T("m2"), _T("m3"), _T("M3"), _T("M3"), _T("m7"), _T("p8") };
    //for(i=0; i < 8; i++) {
    //    wxLogMessage(_T("Intv1='%s', intv2='%s' --> sum='%s'"),
    //        sIntv1[i].c_str(), sIntv2[i].c_str(), AddIntervals(sIntv1[i], sIntv2[i]).c_str() );
    //}

    ////Contructor from lmFiguredBass
    //wxLogMessage(_T("[lmChord::UnitTests] Contructor from lmFiguredBass:"));
    //wxLogMessage(_T("==========================================================\n"));
    //for(i=0; i < 8; i++)
    //{
    //    lmFiguredBass* pFB = new lmFiguredBass((lmVStaff*)NULL, 
    //    wxLogMessage(_T("Intv1='%s', intv2='%s' --> sum='%s'"),
    //        sIntv1[i].c_str(), sIntv2[i].c_str(), AddIntervals(sIntv1[i], sIntv2[i]).c_str() );
    //}


}
#endif  // __WXDEBUG__

//----------------------------------------------------------------------------------------
//global functions
//----------------------------------------------------------------------------------------

wxString ChordTypeToName(lmEChordType nType)
{
    if (nType >= ect_Max)
        return _("Not identified");

    //language dependent strings. Can not be statically initiallized because
    //then they do not get translated
    if (!m_fStringsInitialized) {
        // Triads
        m_sChordName[ect_MajorTriad] = _("Major triad");
        m_sChordName[ect_MinorTriad] = _("Minor triad");
        m_sChordName[ect_AugTriad] = _("Augmented triad");
        m_sChordName[ect_DimTriad] = _("Diminished triad");
        m_sChordName[ect_Suspended_4th] = _("Suspended triad (4th)");
        m_sChordName[ect_Suspended_2nd] = _("Suspended triad (2nd)");

        // Seventh chords
        m_sChordName[ect_MajorSeventh] = _("Major 7th");
        m_sChordName[ect_DominantSeventh] = _("Dominant 7th");
        m_sChordName[ect_MinorSeventh] = _("Minor 7th");
        m_sChordName[ect_DimSeventh] = _("Diminished 7th");
        m_sChordName[ect_HalfDimSeventh] = _("Half diminished 7th");
        m_sChordName[ect_AugMajorSeventh] = _("Augmented major 7th");
        m_sChordName[ect_AugSeventh] = _("Augmented 7th");
        m_sChordName[ect_MinorMajorSeventh] = _("Minor major 7th");

        // Sixth chords
        m_sChordName[ect_MajorSixth] = _("Major 6th");
        m_sChordName[ect_MinorSixth] = _("Minor 6th");
        m_sChordName[ect_AugSixth] = _("Augmented 6th");

        m_fStringsInitialized = true;
    }

    return m_sChordName[nType];

}

int NumNotesInChord(lmEChordType nChordType)
{
    wxASSERT(nChordType < ect_Max);
    return tData[nChordType].nNumNotes;

}

lmEChordType ChordShortNameToType(wxString sName)
{
    // returns -1 if error
    //
    // m-minor, M-major, a-augmented, d-diminished, s-suspended
    // T-triad, dom-dominant, hd-half diminished
    //
    // triads: mT, MT, aT, dT, s4, s2
    // sevenths: m7, M7, a7, d7, mM7, aM7 dom7, hd7
    // sixths: m6, M6, a6

            // Triads
    if      (sName == _T("MT")) return ect_MajorTriad;
    else if (sName == _T("mT")) return ect_MinorTriad;
    else if (sName == _T("aT")) return ect_AugTriad;
    else if (sName == _T("dT")) return ect_DimTriad;
    else if (sName == _T("s4")) return ect_Suspended_4th;
    else if (sName == _T("s2")) return ect_Suspended_2nd;

        // Seventh chords
    else if (sName == _T("M7")) return ect_MajorSeventh;
    else if (sName == _T("dom7")) return ect_DominantSeventh;
    else if (sName == _T("m7")) return ect_MinorSeventh;
    else if (sName == _T("d7")) return ect_DimSeventh;
    else if (sName == _T("hd7")) return ect_HalfDimSeventh;
    else if (sName == _T("aM7")) return ect_AugMajorSeventh;
    else if (sName == _T("a7")) return ect_AugSeventh;
    else if (sName == _T("mM7")) return ect_MinorMajorSeventh;

        // Sixth chords
    else if (sName == _T("M6")) return ect_MajorSixth;
    else if (sName == _T("m6")) return ect_MinorSixth;
    else if (sName == _T("a6")) return ect_AugSixth;

    return (lmEChordType)-1;  //error

}