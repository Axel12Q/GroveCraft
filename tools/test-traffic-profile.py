import unittest
from traffic_profile import build,vehicles

class TrafficPolicyTest(unittest.TestCase):
    def test_native_whitespace_rows_and_special_vehicle_rejection(self):
        catalogue=vehicles('586 wayfarer wayfarer bike WAYFARER WAYFARE bike motorbike 6\n495,sandking,sandking,car,SANDKING,SANDKIN,null,ignore,4\n400,landstal,landstal,car,L,LANDST,null,normal,10')
        self.assertEqual(catalogue['wayfarer']['id'],586)
        policy={'version':1,'groups':{'FARM':{'index':0,'add':['sandking']}}}
        with self.assertRaisesRegex(AssertionError,'Unsupported'):build('landstal # FARM\n','495,sandking,sandking,car,S,S,null,ignore,4',policy)

    def test_capacity_failure_leaves_input_unchanged(self):
        original=', '.join(['base']*23)+' # FARM\r\n'
        policy={'version':1,'groups':{'FARM':{'index':0,'add':['newcar']}}}
        with self.assertRaisesRegex(AssertionError,'capacity'):build(original,'400,newcar,newcar,car,N,N,null,normal,10',policy)
        self.assertEqual(original.count('base'),23)

    def test_only_selected_group_changes_and_order_mismatch_fails(self):
        original='# own fixture\r\nbase # FARM\r\nbase, base # GANG\r\n'
        ide='400,base,base,car,B,B,null,normal,10\n401,newcar,newcar,car,N,N,null,normal,10'
        policy={'version':1,'groups':{'FARM':{'index':0,'add':['newcar']}}}
        text,changes=build(original,ide,policy)
        self.assertTrue(text.endswith('base, base # GANG\r\n'));self.assertEqual(changes[0]['added_ids'],[401])
        policy['groups']['FARM']['index']=1
        with self.assertRaisesRegex(AssertionError,'order'):build(original,ide,policy)

if __name__=='__main__':unittest.main()
