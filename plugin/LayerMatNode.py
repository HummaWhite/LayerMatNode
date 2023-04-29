import maya.mel
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AELayerMatNodeTemplate(ShaderAETemplate):
    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.beginLayout('Parameters', collapse=False)
        self.addControl('thickness', label='Layer Thickness')
        self.addControl('g', label='G')
        self.addControl('albedo', label='Albedo')
        self.addControl('top_normal', label='Top Normal')
        self.addControl('bottom_normal', label='Bottom Normal')
        self.endLayout()

        self.suppress('normal_camera')
        self.suppress('hardware_color')

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)

        self.addExtraControls()
        self.endScrollLayout()
