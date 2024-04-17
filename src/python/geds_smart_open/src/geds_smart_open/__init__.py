#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

from .pygeds import GEDS
from smart_open.transport import register_transport

from . import geds
from .geds import register_object_store
from .geds import relocate

register_transport(geds)

__all__ = ["GEDS", "register_object_store", "relocate"]
