#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pkgutil, traceback
class Log(object):
    @staticmethod
    def error(*args, **kwargs):
        print args, kwargs

PROPERTY_NAME = '__plugin_factory_registry__'
METAPROPERTY  = '__plugin__name__attribute__'

class PluginFactory(type):
    def __init__(cls, class_name, parents, attrs):
        """
        Args:
            cls: The class type we're registering.
            class_name: A String containing the class_name.
            parents: The parent class types of 'cls'.
            attrs: The attribute (members) of 'cls'.
        """
        type.__init__(cls, class_name, parents, attrs)

        # Be sure the property storing the mapping exists
        try:
            registry = getattr(cls, PROPERTY_NAME)
        except AttributeError:
            setattr(cls, PROPERTY_NAME, {})
            registry = getattr(cls, PROPERTY_NAME)

        if not '__metaclass__' in attrs:
            # We are not in the base class: register plugin
            plugin_name_attribute = cls.get_plugin_name_attribute()
            plugin_names = attrs.get(plugin_name_attribute, [])

            if not isinstance(plugin_names, list):
                plugin_names = [plugin_names]

            for plugin_name in plugin_names:
                if plugin_name in registry.keys():
                    raise KeyError("Colliding plugin_name (%s) for class %s" % (plugin_name, cls))
                registry[plugin_name] = cls #class_name

        else:
            # We are in the base class
            cls.plugin_name_attribute = attrs.get(METAPROPERTY, '__name__')

            # Adding a class method get to retrieve plugins by name
            def factory_get(self, name):
                """
                Retrieve a registered Gateway according to a given name.
                Args:
                    name: The name of the Gateway (gateway_type in the Storage).
                Returns:
                    The corresponding Gateway.
                """
                try:
                    return registry[name]
                except KeyError:
                    import traceback
                    traceback.print_exc()
                    Log.error("Cannot find %s in {%s}" % (name, ', '.join(registry.keys())))

            setattr(cls, 'factory_get', classmethod(factory_get))
            setattr(cls, 'factory_list', classmethod(lambda self: registry))
            setattr(cls, 'get_plugin_name_attribute', classmethod(lambda self: self.plugin_name_attribute))

    @staticmethod
    def register(package):
        """
        Register a module in Manifold.
        Args:
            package: a module instance.
        """
        prefix = package.__name__ + "."
        # Explored modules are automatically imported by walk_modules + it allows to explore
        # recursively manifold/gateways/
        # http://docs.python.org/2/library/pkgutil.html
        for importer, modname, ispkg in pkgutil.walk_packages(package.__path__, prefix, onerror = None):
            try:
                module = __import__(modname, fromlist = "dummy")
            except Exception, e:
                Log.warning("Cannot load %s : %s: %s" % (modname, e, traceback.format_exc()))
