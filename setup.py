"""seed: shared evaluation environment for sdn
"""

from setuptools import setup

setup(name='seed',
      version='0.1.2',
      description='shared evaluation environment for sdn',
      url='https://git.scc.kit.edu/pdf-tm-ws1617/seed',
      author='Addis Dittebrandt, Michael Koenig, Felix Neumeister',
      author_email='addis.dittebrandt@student.kit.edu',
      license='MIT',
      packages=['seed'],
      install_requires=[
          'docker==2.5.0',
          'PyYAML==3.12'
      ],
      zip_safe=True,
      entry_points={
          'console_scripts':[
              'seed=seed.__main__:main'
          ]
      })
