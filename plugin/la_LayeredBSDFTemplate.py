import maya.mel
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AEla_LayeredBSDFTemplate(ShaderAETemplate):
    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.addControl('thickness', label='Layer Thickness')
        self.addControl('g', label='G')
        self.addControl('albedo', label='Albedo')

        self.beginLayout('Top BSDF', collapse=False)
        self.addControl('top_node', label='Top BSDF')
        self.addControl('top_normal', label='Top Normal')
        self.addControl('top_correct_normal', label='Gamma Correct')
        self.addControl('top_flip_normal', label='Flip Normal')
        self.endLayout()

        self.beginLayout('Bottom BSDF', collapse=False)
        self.addControl('bottom_node', label='Bottom BSDF')
        self.addControl('bottom_normal', label='Bottom Normal')
        self.addControl('bottom_correct_normal', label='Gamma Correct')
        self.addControl('bottom_flip_normal', label='Flip Normal')
        self.endLayout()

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)
        self.addExtraControls()
        self.endScrollLayout()
