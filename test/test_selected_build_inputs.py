"""Exercise the actual SDK SCons graph; never compile or replace an SDK archive."""
from pathlib import Path
import subprocess
import tempfile
import unittest

SDK = Path(__file__).resolve().parents[1]
CUSTOM = SDK.parent / 'thirdparty/godot_api'


class SelectedBuildInputsTest(unittest.TestCase):
    def run_graph(self, options, expected_api, expected_interface, reject_duplicate=False):
        with tempfile.TemporaryDirectory(prefix='ange-sdk-input-graph-') as scratch:
            script = Path(scratch) / 'SConstruct'
            script.write_text('''
from pathlib import Path
env = SConscript(%r)
selected = env.get('GODOT_CPP_SELECTED_BUILD_INPUTS')
assert isinstance(selected, tuple), 'missing immutable selected build inputs'
assert selected.schema == 1
assert Path(selected.api_node.abspath) == Path(%r)
assert Path(selected.interface_node.abspath) == Path(%r)
assert selected.archive_node is not None
assert list(env['LIBS']).count(selected.archive_node) == 1
assert selected.archive_node.name.startswith('libgodot-cpp')
try:
    selected.schema = 2
    raise AssertionError('selected inputs must be immutable')
except AttributeError:
    pass
if %r:
    from SCons.Errors import UserError
    env.Append(LIBS=[selected.archive_node])
    try:
        SConscript(%r, exports={"env": env})
    except UserError as error:
        assert 'exactly once' in str(error), repr(error)
    else:
        raise AssertionError('duplicate linked SDK archive must refuse publication')
print('SELECTED_BUILD_INPUTS_OK')
Default(Alias('selected-input-check'))
''' % (str(SDK / 'SConstruct'), str(expected_api), str(expected_interface), reject_duplicate, str(SDK / 'SConstruct')))
            result = subprocess.run(['scons', '-Q', '-n', '-f', str(script),
                                     'api_version=4.6', *options], cwd=scratch,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn('SELECTED_BUILD_INPUTS_OK', result.stdout)

    def test_default_selection_when_building_archive(self):
        self.run_graph(['build_library=yes'], SDK/'gdextension/extension_api-4-6.json',
                       SDK/'gdextension/gdextension_interface.json')

    def test_existing_archive_is_exported_when_build_is_disabled(self):
        self.run_graph(['build_library=no'], SDK/'gdextension/extension_api-4-6.json',
                       SDK/'gdextension/gdextension_interface.json')

    def test_duplicate_linked_archive_is_rejected(self):
        self.run_graph(['build_library=no'], SDK/'gdextension/extension_api-4-6.json',
                       SDK/'gdextension/gdextension_interface.json', reject_duplicate=True)

    def test_custom_api_and_interface_follow_actual_selection(self):
        self.run_graph(['build_library=no', 'gdextension_dir='+str(CUSTOM),
                        'custom_api_file='+str(SDK/'gdextension/extension_api-4-5.json')],
                       SDK/'gdextension/extension_api-4-5.json', CUSTOM/'gdextension_interface.json')


if __name__ == '__main__':
    unittest.main()
