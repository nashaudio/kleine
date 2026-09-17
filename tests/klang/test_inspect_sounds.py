"""Behavioural checks for the audio report's all-channel comparison policy."""
import unittest
from inspect_sounds import compare, levels
import numpy as np


class AudioComparison(unittest.TestCase):
    def test_silence_is_valid(self):
        self.assertTrue(compare(np.zeros((4,2)),np.zeros((4,2)))['sample_identical'])

    def test_right_channel_changes_are_detected(self):
        a=np.zeros((4,2)); b=a.copy(); b[3,1]=.5
        result=compare(a,b)
        self.assertFalse(result['equivalent'])
        self.assertEqual(result['residual_peak'],.5)
        self.assertAlmostEqual(result['residual_rms'],(.25/8)**.5)

    def test_neither_alignment_nor_gain_matching(self):
        a=np.array([[0.],[1.],[0.]])
        self.assertFalse(compare(a,np.roll(a,1,axis=0))['equivalent'])
        self.assertFalse(compare(a,a*.5)['equivalent'])

    def test_shape_and_invalid_audio(self):
        self.assertFalse(compare(np.zeros((4,2)),np.zeros((8,1)))['equivalent'])
        self.assertFalse(compare(np.zeros((4,1)),np.zeros((5,1)))['equivalent'])
        self.assertFalse(compare(np.empty((0,1)),np.empty((0,1)))['equivalent'])
        self.assertFalse(compare(np.array([[np.nan]]),np.array([[np.nan]]))['equivalent'])

    def test_explicit_tolerance_does_not_claim_identical(self):
        result=compare(np.ones((4,1)),np.ones((4,1))+.00001,atol=.00002)
        self.assertTrue(result['equivalent'])
        self.assertFalse(result['sample_identical'])

    def test_partial_level_bin_uses_actual_count(self):
        time,rms,peak=levels(np.array([1.,-1.,.5]),200)
        np.testing.assert_array_equal(rms,[1.,.5])
        np.testing.assert_array_equal(peak,[1.,.5])
        self.assertEqual(len(time),2)


if __name__=='__main__': unittest.main()
