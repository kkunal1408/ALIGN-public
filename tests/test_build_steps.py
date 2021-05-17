import pytest
from align.main import build_steps

def test_A():
    assert ['1_topology','2_primitives', '3_pnr:prep' , '3_pnr:place', '3_pnr:route', '3_pnr:check'] == build_steps( '1_topology', '3_pnr')

    assert ['1_topology','2_primitives', '3_pnr:prep' , '3_pnr:place', '3_pnr:route'] == build_steps( '1_topology', '3_pnr:route')

    assert ['1_topology','2_primitives', '3_pnr:prep'] == build_steps( '1_topology', '3_pnr:prep')

    assert [ '3_pnr:prep'] == build_steps( '3_pnr:prep', '3_pnr:prep')

    with pytest.raises(AssertionError):
        build_steps( '3_pnr:prep', '1_topology')

    with pytest.raises(NotImplementedError):
        build_steps( None, '3_pnr:place')

    with pytest.raises(NotImplementedError):
        build_steps( '3_pnr:route', None)

    with pytest.raises(NotImplementedError):
        build_steps( '3_pnr:check', None)

