@# Included from rosidl_generator_py/resource/_idl_experimental.py.em
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=action.goal)
}@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=action.result)
}@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback)
}@
@{
TEMPLATE(
    '_msg_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    message=action.feedback_message)
}@
@{
TEMPLATE(
    '_srv_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    service=action.send_goal_service)
}@
@{
TEMPLATE(
    '_srv_experimental.py.em',
    package_name=package_name, interface_path=interface_path,
    service=action.get_result_service)
}@


class @(action.namespaced_type.name):
    """Experimental action class '@(action.namespaced_type.name)'."""

    Goal = @(action.goal.structure.namespaced_type.name)
    Result = @(action.result.structure.namespaced_type.name)
    Feedback = @(action.feedback.structure.namespaced_type.name)
    FeedbackMessage = @(action.feedback_message.structure.namespaced_type.name)
    SendGoal = @(action.send_goal_service.namespaced_type.name)
    GetResult = @(action.get_result_service.namespaced_type.name)
