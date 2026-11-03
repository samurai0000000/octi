/*
 * BotTuningProfile.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class BotTuningProfile {

    // --- 1. Search Horizon, Quiescence & Concurrency ---
    public int timeBudgetMs = 8000;
    public int maxSearchDepth = 6;
    public int maxQuiesceDepth = 2;
    public int quiesceBranchCap = 6;
    public int rootBranchCap = 36;
    public int treeBranchCap = 24;
    public int lazySmpThreads = 2;
    public int compositeMoveMaxPly = 1;
    public int rootScoreJitter = 25;

    // --- 2. Base Occupation & Distance Weights ---
    public int capturedEnemyBaseBonus = 28000;
    public int lostAlliedBasePenalty = 32000;
    public int neutralOctiSquareBonus = 4000;
    public int closestPodDistWeight = 320;
    public int secondClosestPodDistWeight = 140;
    public int oneStepBaseThreatBonus = 8500;
    public int oneJumpBaseThreatBonus = 12000;
    public int twoPlyBaseThreatWeight = 2400;

    // --- 3. Multi-Pod Development & Anti-Monopoly Weights ---
    public int podZeroProngVal = 320;
    public int podOneProngVal = 1350;
    public int podTwoProngVal = 2100;
    public int podThreePlusProngVal = 2800;
    public int normalProngVal = 380;
    public int superProngVal = 550;
    public int orthDiagVersatilityBonus = 550;
    public int diagOnlyParityPenalty = 350;
    public int firstArmedPodBonus = 600;
    public int secondArmedPodBonus = 1100;
    public int thirdArmedPodBonus = 700;
    public int singlePodMonopolyPenalty = 1000;
    public int loneOneProngRushPenalty = 550;

    // --- 4. Leapfrog / Launchpad Battery & Jump Reachability Weights ---
    public int leapfrogBatteryWeight = 550;
    public int leapfrogForwardBonus = 400;
    public int jumpReachableSquareWeight = 130;
    public int jumpReachStepWeight = 450;
    public int jumpCapturablePodWeight = 600;
    public int jumpCapturableProngWeight = 120;

    // --- 5. Hanging-Pod Safety, OFK Prong-Seizure & Move Ordering ---
    public int hangingPodBasePenalty = 1400;
    public int hangingProngWeight = 220;
    public int ofkProngSeizureWeight = 450;
    public int opponentTurnHangingPct = 125;
    public int soleArmedPodDangerPenalty = 3500;
    public int wipeoutDangerPenalty = 15000;
    public int moveOrderCaptureBonus = 16000;
    public int moveOrderBaseEntryBonus = 60000;
    public int moveOrderLeapfrogBonus = 1200;
    public int moveOrderBlunderPenalty = 7000;

    public BotTuningProfile() {}

    public BotTuningProfile(BotTuningProfile other) {
        if (other != null) {
            this.timeBudgetMs = other.timeBudgetMs;
            this.maxSearchDepth = other.maxSearchDepth;
            this.maxQuiesceDepth = other.maxQuiesceDepth;
            this.quiesceBranchCap = other.quiesceBranchCap;
            this.rootBranchCap = other.rootBranchCap;
            this.treeBranchCap = other.treeBranchCap;
            this.lazySmpThreads = other.lazySmpThreads;
            this.compositeMoveMaxPly = other.compositeMoveMaxPly;
            this.rootScoreJitter = other.rootScoreJitter;

            this.capturedEnemyBaseBonus = other.capturedEnemyBaseBonus;
            this.lostAlliedBasePenalty = other.lostAlliedBasePenalty;
            this.neutralOctiSquareBonus = other.neutralOctiSquareBonus;
            this.closestPodDistWeight = other.closestPodDistWeight;
            this.secondClosestPodDistWeight = other.secondClosestPodDistWeight;
            this.oneStepBaseThreatBonus = other.oneStepBaseThreatBonus;
            this.oneJumpBaseThreatBonus = other.oneJumpBaseThreatBonus;
            this.twoPlyBaseThreatWeight = other.twoPlyBaseThreatWeight;

            this.podZeroProngVal = other.podZeroProngVal;
            this.podOneProngVal = other.podOneProngVal;
            this.podTwoProngVal = other.podTwoProngVal;
            this.podThreePlusProngVal = other.podThreePlusProngVal;
            this.normalProngVal = other.normalProngVal;
            this.superProngVal = other.superProngVal;
            this.orthDiagVersatilityBonus = other.orthDiagVersatilityBonus;
            this.diagOnlyParityPenalty = other.diagOnlyParityPenalty;
            this.firstArmedPodBonus = other.firstArmedPodBonus;
            this.secondArmedPodBonus = other.secondArmedPodBonus;
            this.thirdArmedPodBonus = other.thirdArmedPodBonus;
            this.singlePodMonopolyPenalty = other.singlePodMonopolyPenalty;
            this.loneOneProngRushPenalty = other.loneOneProngRushPenalty;

            this.leapfrogBatteryWeight = other.leapfrogBatteryWeight;
            this.leapfrogForwardBonus = other.leapfrogForwardBonus;
            this.jumpReachableSquareWeight = other.jumpReachableSquareWeight;
            this.jumpReachStepWeight = other.jumpReachStepWeight;
            this.jumpCapturablePodWeight = other.jumpCapturablePodWeight;
            this.jumpCapturableProngWeight = other.jumpCapturableProngWeight;

            this.hangingPodBasePenalty = other.hangingPodBasePenalty;
            this.hangingProngWeight = other.hangingProngWeight;
            this.ofkProngSeizureWeight = other.ofkProngSeizureWeight;
            this.opponentTurnHangingPct = other.opponentTurnHangingPct;
            this.soleArmedPodDangerPenalty = other.soleArmedPodDangerPenalty;
            this.wipeoutDangerPenalty = other.wipeoutDangerPenalty;
            this.moveOrderCaptureBonus = other.moveOrderCaptureBonus;
            this.moveOrderBaseEntryBonus = other.moveOrderBaseEntryBonus;
            this.moveOrderLeapfrogBonus = other.moveOrderLeapfrogBonus;
            this.moveOrderBlunderPenalty = other.moveOrderBlunderPenalty;
        }
    }

    public BotTuningProfile copy() {
        return new BotTuningProfile(this);
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

